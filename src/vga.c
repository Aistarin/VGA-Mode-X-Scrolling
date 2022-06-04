#include "vga.h"
#include <string.h>

byte *VGA = (byte *) 0xA0000;

byte p[5] = {0,2,4,6,8};
byte pix;

void vga_set_mode(byte mode){
    union REGS regs;

    // tell the BIOS to initialize the given mode
	regs.h.ah = 0x00;
	regs.h.al = mode;
	int386(0x10, &regs, &regs);
}

void vga_init_modex()
{
    // initialize mode 13h
    vga_set_mode(0x13);

    // clear memory
    memset(VGA, 0, 0xFFFF);

    /* disable chain 4 */
    word_out(SC_INDEX, 0x04, 0x06);

    /* disable word mode */
    word_out(CRTC_INDEX, 0x17, 0xe3);

    /* disable doubleword mode */
    word_out(CRTC_INDEX, 0x14, 0x00);

    // turn off write protect
    word_out(CRTC_INDEX, V_RETRACE_END, 0x2c);

	//320x240 60Hz
    word_out(CRTC_INDEX, V_TOTAL, 0x0d);
    word_out(CRTC_INDEX, V_OVERFLOW, 0x3e);
    word_out(CRTC_INDEX, V_RETRACE_START, 0xea);
    word_out(CRTC_INDEX, V_RETRACE_END, 0xac);
    word_out(CRTC_INDEX, V_DISPLAY_END, 0xdf);
    word_out(CRTC_INDEX, V_BLANK_START, 0xe7);
    word_out(CRTC_INDEX, V_BLANK_END, 0x06);

    // set logical width of page
    word_out(CRTC_INDEX, V_OFFSET, PAGE_WIDTH >> 3);
}

void vga_wait_for_retrace(){
    /* wait until done with vertical retrace */
    while  ((inp(0x03da) & 0x08)) {};
    /* wait until done refreshing */
    while (!(inp(0x03da) & 0x08)) {};
}

void vga_exit_modex(){
    memset(VGA, 0, 0x4b00);
    // restore text mode
    vga_set_mode(0x03);
}

void vga_set_palette(byte *palette, byte start_index, byte end_index) {
    int i;

    outp(PALETTE_INDEX,0);

    for(i = 0; i < 256*3; i++) {
        outp(PALETTE_DATA, palette[i]);
    }
}

void vga_draw_pixel(word x, word y, byte color){
    dword offset;

    outp(SC_INDEX, MAP_MASK);          /* select plane */
    outp(SC_DATA,  1 << (x&3) );

    offset = ((dword) y * PAGE_WIDTH + x) >> 2;

    VGA[(word)offset]=color;
}

void vga_draw_buffer(byte * buffer, word width, word height, word initial_offset) {
    byte plane;
    word x, y;
    dword offset;
    dword initial_y_offset = initial_offset * PAGE_WIDTH;

    for(plane = 0; plane < 4; plane++) {
        // select the plane bit
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        for(y = 0; y < height; y++) {
            for(x = plane; x < width; x += 4) {
                // TODO: this could probably be reimplemented to be
                // much cache-friendlier by having/assuming that the
                // data in the buffer is already sorted by plane,
                // that way we could just use memcpy to copy an
                // entire plane's worth of data to VRAM in one go
                offset = ((dword) y * PAGE_WIDTH + initial_y_offset + x) >> 2;
                VGA[(word)offset]=buffer[width * y + x];
            }
        }
    }
}

void vga_blit_buffer_to_vram(byte * buffer, word buffer_width, word buffer_height, word source_x, word source_y, word dest_x, word dest_y, word width, word height) {
    byte plane;
    word x, y;
    dword offset;

    for(plane = 0; plane < 4; plane++) {
        // select the plane bit
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        for(y = 0; y < height; y++) {
            for(x = plane; x < width; x += 4) {
                // TODO: this could probably be reimplemented to be
                // much cache-friendlier by having/assuming that the
                // data in the buffer is already sorted by plane,
                // that way we could just use memcpy to copy an
                // entire plane's worth of data to VRAM in one go
                offset = ((dword) (dest_y + y) * PAGE_WIDTH + (dest_x + x)) >> 2;
                VGA[(word)offset]=buffer[buffer_width * (source_y + y) + (source_x + x)];
            }
        }
    }
}

//wait for the VGA to stop drawing, and set scroll and Pel panning
void vga_scroll_offset(word offset_x, word offset_y)
{
    word x = offset_x;
    word y = offset_y;

	byte ac;
    // y = (y * (PAGE_WIDTH >> 2)) + (x>>2);
    y += (x >> 2);

	//change scroll registers: HIGH_ADDRESS 0x0C; LOW_ADDRESS 0x0D
    outpw(CRTC_INDEX, 0x0D | (y << 8));  // low address
    outpw(CRTC_INDEX, 0x0C | (y & 0xff00));  // high address

    // _disable();

    inp(0x03da);
    ac = inp(AC_INDEX);

	//pixel panning value		
	outp(AC_INDEX, 0x33);
	outp(AC_INDEX, p[x & 3]);

    // _enable();

    // outp(AC_INDEX, ac);
}

void vga_blit_vram_to_vram(word source_x, word source_y, word dest_x, word dest_y, word width, word height) {
    // pixel variable made volatile to prevent the compiler from
    // optimizing it out, since value is actually not used
    volatile byte pixel;
    dword source_offset = (((dword)source_y * (dword)PAGE_WIDTH + source_x) >> 2);
    dword dest_offset = (((dword)dest_y * (dword)PAGE_WIDTH + dest_x) >> 2);
    int line, x;

    outpw(SC_INDEX, ((word)0xff << 8) + MAP_MASK);      //select all planes
    outpw(GC_INDEX, 0x08);                              //set to or mode

    for(line = 0; line < height; line++) {
        for(x = 0; x < width >> 2; x++) {
            pixel = VGA[source_offset + x];             //read pixel to load the latches
            VGA[dest_offset + x] = 0;                   //write four pixels
        }
        source_offset += PAGE_WIDTH >> 2;
        dest_offset += PAGE_WIDTH >> 2;
    }

    outpw(GC_INDEX + 1, 0x0ff);
}


void vga_fill_vram_with_color(byte color, word dest_x, word dest_y, word width, word height) {
    dword dest_offset = (((dword)dest_y * (dword)PAGE_WIDTH + dest_x) >> 2);
    int line, x;

    outpw(SC_INDEX, ((word)0xff << 8) + MAP_MASK);      //select all planes
    outpw(GC_INDEX, 0x08);                              //set to or mode

    for(line = 0; line < height; line++) {
        for(x = 0; x < width >> 2; x++) {
            VGA[dest_offset + x] = color;                   //write four pixels
        }
        dest_offset += PAGE_WIDTH >> 2;
    }

    outpw(GC_INDEX + 1, 0x0ff);
}
