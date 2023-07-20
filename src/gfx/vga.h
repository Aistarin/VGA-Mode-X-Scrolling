#ifndef VGA_H_
#define VGA_H_

#include "src/common.h"
#include <conio.h>
#include <i86.h>

#define VGA_START       0xA000  // VGA memory start index

#define SC_INDEX        0x03c4  // VGA sequence controller index
#define SC_DATA         0x03c5

#define PALETTE_INDEX   0x03c8
#define PALETTE_DATA    0x03c9

#define MAP_MASK        0x02

#define CRTC_INDEX      0x03d4  // CRT controller index
#define CRTC_DATA       0x03d5

#define GC_INDEX        0x03ce  // GC index register

#define V_TOTAL         0x06
#define V_OVERFLOW      0x07
#define V_RETRACE_START 0x10
#define V_RETRACE_END   0x11
#define V_DISPLAY_END   0x12
#define V_OFFSET        0x13
#define V_BLANK_START   0x15
#define V_BLANK_END     0x16

#define AC_INDEX        0x03c0  // Attribute controller index

#define SCREEN_WIDTH    320     // Visibile width
#define SCREEN_HEIGHT   240     // Visibile height
#define PAGE_WIDTH      336     // Drawable width, screen width + 1 tile
#define PAGE_HEIGHT     272     // Drawable height, screen height + 2 tiles

/* macro to write a word to a port */
#define word_out(port,register,value) outpw(port,(((word)value<<8) + register))

void vga_init_modex(void);
void vga_exit_modex(void);
void vga_set_mode(byte mode);
void vga_set_palette(byte *palette, byte start_index, byte end_index);
void vga_draw_pixel(word x, word y, byte color);
void vga_draw_buffer(byte * buffer, word width, word height, word initial_offset);
void vga_wait_for_retrace(void);
void vga_scroll_offset(word offset_x, word offset_y);
void vga_blit_vram_to_vram(word source_x, word source_y, word dest_x, word dest_y, word width, word height);
void vga_blit_buffer_to_vram(byte * buffer, word buffer_width, word buffer_height, word source_x, word source_y, word dest_x, word dest_y, word width, word height);
void vga_fill_vram_with_color(byte color, word dest_x, word dest_y, word width, word height);
void vga_set_offset(word offset);
void vga_set_horizontal_pan(byte pan_value);

#endif
