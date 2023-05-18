#include "vga.h"
#include "gfx.h"
#include "spr.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159
#endif

unsigned int COSX[256];
unsigned int SINEY[256];

typedef struct testobj {
    int hspeed;
    int vspeed;
    int xpos;
    int ypos;
} testobj;

void init_sin()
{
  int i;
  for( i = 0;i < 256; ++i ) {
    COSX[i] = SCREEN_WIDTH * (( cos(2.0*M_PI*i/256.0) + 1.0) / 2.0);
    SINEY[i] = SCREEN_HEIGHT * (( sin(2.0*M_PI*i/256.0) + 1.0) / 2.0);
  }
}

void render_pattern_to_buffer_1(byte *screen_buffer, word width, word height) {
    word i, j, color;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            if (!(j % TILE_HEIGHT && i % TILE_WIDTH)) color = 0x40 + (((40 * (i + j)) / (height + width)));
            else color = 0x10 + (((16 * (i + j)) / (height + width)));
            // else color = 0;
            screen_buffer[width * j + i] = color;
        }
    }
}

void render_pattern_to_buffer_2(byte *screen_buffer, word width, word height) {
    word i, j, color;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            if (!(j % TILE_HEIGHT && i % TILE_WIDTH)) color = 0x10 + (((16 * (i + j)) / (height + width)));
            else color = 0x40 + (((40 * (i + j)) / (height + width)));
            // else color = 0;
            screen_buffer[width * j + i] = color;
        }
    }
}

void render_buffer_to_vram_slow(byte *screen_buffer) {
    word i, j;
    for(j = 0; j < PAGE_HEIGHT; j++) {
        for(i = 0; i < PAGE_WIDTH; i++) {
            vga_draw_pixel(i, j, screen_buffer[PAGE_WIDTH * j + i]);
        }
    }
}

void fskip(FILE *fp, int num_bytes)
{
   int i;
   for (i=0; i<num_bytes; i++)
      fgetc(fp);
}

void load_bmp_to_buffer(char *file, byte *screen_buffer, word buffer_width, word buffer_height, byte *palette)
{
    FILE *fp;
    dword index, x;
    word num_colors, width, height;

    /* open the file */
    if ((fp = fopen(file,"rb")) == NULL)
    {
        printf("Error opening file %s.\n",file);
        exit(1);
    }

    /* check to see if it is a valid bitmap file */
    if (fgetc(fp)!='B' || fgetc(fp)!='M')
    {
        fclose(fp);
        printf("%s is not a bitmap file.\n",file);
        exit(1);
    }

    /* read in the width and height of the image, and the
        number of colors used; ignore the rest */
    fskip(fp,16);
    fread(&width, sizeof(word), 1, fp);
    fskip(fp,2);
    fread(&height,sizeof(word), 1, fp);
    fskip(fp,22);
    fread(&num_colors,sizeof(word), 1, fp);
    fskip(fp,6);

    /* assume we are working with an 8-bit file */
    if (num_colors==0) num_colors=256;

    /* read the palette information */
    for(index=0;index<num_colors;index++)
    {
        palette[(int)(index*3+2)] = fgetc(fp) >> 2;
        palette[(int)(index*3+1)] = fgetc(fp) >> 2;
        palette[(int)(index*3+0)] = fgetc(fp) >> 2;
        x=fgetc(fp);
    }

    for(index = 0; index < buffer_height; index++) {
        for(x = 0; x < buffer_width; x++) {
            screen_buffer[(dword) buffer_width * ((dword) buffer_height - index - 1) + x] = (byte)fgetc(fp);
        }
    }

    fclose(fp);
}

int test_scroll(int testobj_max, bool jodi){
    int i, j, k, offset, x, y, tile_offset_x = 0, tile_offset_y = 0, pos_x = 0, pos_y = 0;
    gfx_buffer *tileset_buffer;
    gfx_buffer *sprite_buffer;
    gfx_buffer *test_buffer;
    gfx_tilemap *tilemap_buffer;
    byte palette[256*3];
    char ch;
    int testobj_count = 0;
    dword compiled_sized = 0;

    testobj *testobj_list = malloc(sizeof(testobj) * testobj_max);
    testobj *cur_testobj;

    test_buffer = gfx_create_empty_buffer(0, 64, 32, FALSE, 0);
    load_bmp_to_buffer("dvd-logo.bmp", test_buffer->buffer, test_buffer->width, test_buffer->height, palette);
    compiled_sized = spr_compile_planar_sprite(test_buffer->buffer, test_buffer->width, test_buffer->height, NULL, NULL);

    sprite_buffer = gfx_create_empty_buffer(0, test_buffer->width, test_buffer->height, TRUE, compiled_sized);
    spr_compile_planar_sprite(test_buffer->buffer, test_buffer->width, test_buffer->height, sprite_buffer->buffer, sprite_buffer->plane_offsets);

    // gfx_load_linear_bitmap_to_planar_bitmap(test_buffer->buffer, sprite_buffer->buffer, sprite_buffer->width, sprite_buffer->height);

    gfx_init_video();

    palette[0] = 0;
    palette[1] = 0;
    palette[2] = 0;
    vga_set_palette(palette, 0, 15);

    tileset_buffer = gfx_get_tileset_buffer();
    if(jodi) {
        load_bmp_to_buffer("jodi.bmp", tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height, palette);
        vga_set_palette(palette, 0, 255);
    } else {
        render_pattern_to_buffer_1(tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height);
    }

    gfx_load_tileset();

    tilemap_buffer = gfx_get_tilemap_buffer();

    for(i = 0; i < 256; i++){
        x = i % 16;
        y = i / 16;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x] = (byte) i;
        tilemap_buffer->buffer[(y + 16) * tilemap_buffer->horz_tiles + x] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 16] = (byte) i;
        tilemap_buffer->buffer[(y + 16) * tilemap_buffer->horz_tiles + x + 16] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 32] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 48] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 64] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 80] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 96] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 112] = (byte) i;
    }

    for(i = 0; i < 336; i++) {
        x = i % 21;
        y = i / 21;
        gfx_set_tile(tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x], x, y);
    }

    pos_x = 0;
    pos_y = 0;
    tile_offset_x = 0;
    tile_offset_y = 0;
    i = 0;
    while (1) {
        if(kbhit()) {
            ch = getch();
            if(ch == 27)
                break;
            else if(ch == 119)  // w
                pos_y -= 1;
            else if(ch == 115)  // s
                pos_y += 1;
            else if(ch == 97)   // a
                pos_x -= 1;
            else if(ch == 100)  // d
                pos_x += 1;
            else if(ch == 113) { // q
                pos_x -= 1;
                pos_y -= 1;
            } else if(ch == 101) { // E
                pos_x += 1;
                pos_y += 1;
            }
            else if(ch == 87)  // W
                pos_y -= TILE_HEIGHT;
            else if(ch == 83)  // S
                pos_y += TILE_HEIGHT;
            else if(ch == 65)   // A
                pos_x -= TILE_WIDTH;
            else if(ch == 68)  // D
                pos_x += TILE_WIDTH;
            else if(ch == 81) { // Q
                pos_x -= TILE_WIDTH;
                pos_y -= TILE_HEIGHT;
            } else if(ch == 69) { // E
                pos_x += TILE_WIDTH;
                pos_y += TILE_HEIGHT;
            }
        }

        if(pos_y < 0)
            pos_y = 0;
        if(pos_x < 0)
            pos_x = 0;

        for(j = 0; j < testobj_count; j++) {
            cur_testobj = &testobj_list[j];
            cur_testobj->xpos += cur_testobj->hspeed;
            cur_testobj->ypos += cur_testobj->vspeed;
            if(cur_testobj->xpos > (320 - sprite_buffer->width) || cur_testobj->xpos < 0) {
                if(cur_testobj->xpos > (320 - sprite_buffer->width)) cur_testobj->xpos = (320 - sprite_buffer->width);
                else if(cur_testobj->xpos < 0) cur_testobj->xpos = 0;
                cur_testobj->hspeed = -(cur_testobj->hspeed);
            }

            if(cur_testobj->ypos > (240 - sprite_buffer->height) || cur_testobj->ypos < 0) {
                if(cur_testobj->ypos > (240 - sprite_buffer->height)) cur_testobj->ypos = (240 - sprite_buffer->height);
                else if(cur_testobj->ypos < 0) cur_testobj->ypos = 0;
                cur_testobj->vspeed = -(cur_testobj->vspeed);
            }
        }
        if(k++ % 30 == 0 && testobj_count < testobj_max){
            cur_testobj = &testobj_list[testobj_count++];
            cur_testobj->xpos = rand() % (320 - sprite_buffer->width);
            cur_testobj->ypos = rand() % (240 - sprite_buffer->height);
            cur_testobj->hspeed = 1 + rand() % 5;
            cur_testobj->vspeed = 1 + rand() % 5;
        }
        /* scroll screen before drawing sprites */
        gfx_set_scroll_offset(pos_x, pos_y);
        for(j = 0; j < testobj_count; j++) {
            cur_testobj = &testobj_list[j];
            gfx_draw_sprite_to_screen(sprite_buffer, 0, 0, (word) cur_testobj->xpos + (pos_x % TILE_WIDTH), (word) cur_testobj->ypos + (pos_y % TILE_HEIGHT), sprite_buffer->width, sprite_buffer->height);
        }
        gfx_render_all();
    }

    vga_exit_modex();

    printf("total objects rendered: %d\n", testobj_count);

    return 0;
}

int main(int argc, char *argv[]) {
    char *a = argv[1];
    char *b = argv[2];
    int num = atoi(a);
    int jodi = atoi(b);

    if(argc >= 2)
        return test_scroll(num, jodi);
    else
        return test_scroll(256, FALSE);
}
