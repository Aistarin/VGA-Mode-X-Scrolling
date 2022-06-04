#include "vga.h"
#include "gfx.h"
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
    long index;
    word num_colors, width, height;
    int x;

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

    for(index = (long) buffer_height; index > 0; index--) {
        for(x = 0; x < (int) buffer_width; x++) {
            screen_buffer[buffer_width * index + x] = (byte)fgetc(fp);
        }
    }

    fclose(fp);
}

int scroll_image() {
    word abs_y_offset = 0;
    word vrt_y_offset = 0;
    byte *image_buffer = malloc(sizeof(byte) * 320 * 960);
    byte palette[256*3];
    byte temp_col[3];
    byte colors_to_cycle[4] = {60, 96, 101, 108};

    vga_init_modex();
    load_bmp_to_buffer("magfest.bmp", image_buffer, 320, 960, palette);

    vga_set_palette(palette, 0, 255);

    vga_blit_buffer_to_vram(image_buffer, 320, 960, 0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    vga_scroll_offset(0, 0);

    while (!_kbhit()){
        vga_set_palette(palette, 0, 255);
        // this waits for a retrace before setting the offset
        vga_scroll_offset(0, abs_y_offset);

        // populate row that we just scrolled off of with the new one so that
        // it can be ready for us when we are ready to page flip
        if(abs_y_offset % 16 == 0) {
            vga_blit_buffer_to_vram(image_buffer, 320, 960, 0, vrt_y_offset + SCREEN_HEIGHT, 0, abs_y_offset + SCREEN_HEIGHT, PAGE_WIDTH, 16);
            if (abs_y_offset != 0) vga_blit_vram_to_vram(0, abs_y_offset + SCREEN_HEIGHT - 16, 0, abs_y_offset - 16, PAGE_WIDTH, 16);

            // cycle colors
            memcpy(temp_col, &palette[108 * 3], sizeof(byte) * 3);
            memcpy(&palette[108 * 3], &palette[101 * 3], sizeof(byte) * 3);
            memcpy(&palette[101 * 3], &palette[96 * 3], sizeof(byte) * 3);
            memcpy(&palette[96 * 3], &palette[60 * 3], sizeof(byte) * 3);
            memcpy(&palette[60 * 3], temp_col, sizeof(byte) * 3);
        };

        abs_y_offset++;
        vrt_y_offset++;

        if(abs_y_offset == SCREEN_HEIGHT) {
            // once we reach the bottom of the visible page, we have to
            // manually copy the last row up before we flip back
            vga_blit_vram_to_vram(0, (SCREEN_HEIGHT * 2 ) - 16, 0, SCREEN_HEIGHT - 16, PAGE_WIDTH, 16);
            abs_y_offset = 0;
        }

        if(vrt_y_offset == 720) {
            // set the virtual y offset to negative screen height so that
            // the modulo logic will begin copying the rows starting from
            // the beginning
            vrt_y_offset = SCREEN_HEIGHT * -1;
        }        
    }
    vga_exit_modex();

    free(image_buffer);
    free(palette);

    return 0;
}

int test_pattern_routine(){
    word i, j, k;
    word bounce = 1;
    byte color = 0;

    byte *screen_buffer_1 = malloc(sizeof(byte) * PAGE_WIDTH * PAGE_HEIGHT);
    byte *screen_buffer_2 = malloc(sizeof(byte) * PAGE_WIDTH * PAGE_HEIGHT);
    byte *tile_buffer_1 = malloc(sizeof(byte) * 256 * 256);
    byte *tile_buffer_2 = malloc(sizeof(byte) * 256 * 256);

    init_sin();
    // TODO: use render pattern to create a tileset!
    render_pattern_to_buffer_1(screen_buffer_1, PAGE_WIDTH, PAGE_HEIGHT);
    render_pattern_to_buffer_2(screen_buffer_2, PAGE_WIDTH, PAGE_HEIGHT);
    render_pattern_to_buffer_1(tile_buffer_1, 256, 256);
    render_pattern_to_buffer_2(tile_buffer_2, 256, 256);

    vga_init_modex();

    vga_scroll_offset(0, 0);

    // load_bmp_to_buffer("jodiflpy.bmp", screen_buffer_1);
    // vga_set_palette(palette, 0, 255);

    // vga_draw_buffer(screen_buffer_1, PAGE_WIDTH, PAGE_HEIGHT, 0);
    // vga_draw_buffer(screen_buffer_2, PAGE_WIDTH, PAGE_HEIGHT, PAGE_HEIGHT);

    // vga_draw_buffer(tile_buffer_1, 256, 256, PAGE_HEIGHT);
    vga_draw_buffer(screen_buffer_1, PAGE_WIDTH, PAGE_HEIGHT, PAGE_HEIGHT);
    vga_draw_buffer(screen_buffer_2, PAGE_WIDTH, PAGE_HEIGHT, PAGE_HEIGHT * 2);

    // vga_blit_vram_to_vram(128, PAGE_HEIGHT + 128, 0, 0, 16, 16);

    while (!_kbhit()){
        // i = rand() % 335;
        // j = rand() % 255;
        // color = rand() % 256;
        // vga_draw_pixel(i, j, color);

        i = (172 * COSX[(int) k]) / 256;
        j = (172 * SINEY[(int) k]) / 256;

        vga_scroll_offset(64 + i, 256 + j);

        // vga_scroll_offset(0, 0);

        // k = k + bounce;

        // if (k == 16) bounce = -1;
        // else if (k == 0) bounce = 1;

        k += 1;
        
        if(k >= 256) {
            k = 0;
            // if (bounce) {
            //     vga_draw_buffer(screen_buffer_2, PAGE_WIDTH, PAGE_HEIGHT);
            //     bounce = 0;
            // } else {
            //     vga_draw_buffer(screen_buffer_1, PAGE_WIDTH, PAGE_HEIGHT);
            //     bounce = 1;
            // }
        }

        // if (k % 2 == 0) {
        //     vga_blit_vram_to_vram(0, PAGE_HEIGHT, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);
        // } else {
        //     vga_blit_vram_to_vram(0, PAGE_HEIGHT * 2, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);
        // }
    }
    vga_exit_modex();

    free(tile_buffer_1);
    free(screen_buffer_1);
    free(screen_buffer_2);

    return 0;
}

int test_tile_routine(int testobj_max){
    word i, j, k;
    gfx_buffer *screen_buffer;
    gfx_buffer *tileset_buffer;
    gfx_buffer *sprite_buffer;
    gfx_buffer *test_buffer;
    byte palette[256*3];

    int testobj_count = 0;

    testobj *testobj_list = malloc(sizeof(testobj) * testobj_max);
    testobj *cur_testobj;

    gfx_init_video();

    screen_buffer = gfx_get_screen_buffer();
    tileset_buffer = gfx_get_tileset_buffer();
    test_buffer = gfx_create_empty_buffer(0, TILE_WIDTH, TILE_HEIGHT, FALSE);
    sprite_buffer = gfx_create_empty_buffer(0, test_buffer->width, test_buffer->height, TRUE);

    // for(i=0;i<sprite_buffer->width * sprite_buffer->height;i++)
    //     sprite_buffer->buffer[i] = 15;

    // load_bmp_to_buffer("dvd-logo.bmp", test_buffer->buffer, 64, 32, palette);
    // vga_set_palette(palette, 0, 255);

    render_pattern_to_buffer_2(test_buffer->buffer, test_buffer->width, test_buffer->height);
    render_pattern_to_buffer_1(tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height);
    gfx_load_linear_bitmap_to_planar_bitmap(test_buffer->buffer, sprite_buffer->buffer, test_buffer->width, test_buffer->height);

    gfx_load_tileset();

    for(i = 0; i < 32; i++)
        gfx_set_tile(0, i % 2, i / 2);

    for(i = 0; i < 48; i++)
        gfx_set_tile(0, i % 3 + 18, i / 3);

    // main program loop
    i = 0;
    j = 0;
    k = 0;
    while (!_kbhit()){
        if(i < 256){
            gfx_set_tile(i, i % 16 + 2, i / 16);
            i++;
        }
        else {
            for(j = 0; j < testobj_count; j++) {
                cur_testobj = &testobj_list[j];
                gfx_draw_sprite_to_screen(sprite_buffer, 0, 0, (word) cur_testobj->xpos, (word) cur_testobj->ypos, sprite_buffer->width, sprite_buffer->height);

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
        }
        gfx_render_all();
    }

    vga_exit_modex();

    printf("total objects rendered: %d\n", testobj_count);

    // free(tile_buffer);
    // free(screen_buffer);

    return 0;
}

int test_vga(){
    int i, j, index;
    byte palette[256*3];

    gfx_init_video();

    index = 1;

    palette[(int)(index*3+2)] = 255;
    palette[(int)(index*3+1)] = 0;
    palette[(int)(index*3+0)] = 0;

    vga_set_palette(palette, 0, 255);

    for(i=0;i<PAGE_WIDTH;i++)
        for(j=0;j<PAGE_HEIGHT;j++)
            vga_draw_pixel(i, j, 1);

    while (!_kbhit()){
        vga_wait_for_retrace();
    }

    vga_exit_modex();

    return 0;
}

int test_scroll(int testobj_max){
    int i, j, k, offset, x, y, tile_offset_x = 0, tile_offset_y = 0, pos_x = 0, pos_y = 0;
    gfx_buffer *tileset_buffer;
    gfx_buffer *sprite_buffer;
    byte palette[256*3];
    char ch;
    int testobj_count = 0;

    testobj *testobj_list = malloc(sizeof(testobj) * testobj_max);
    testobj *cur_testobj;

    sprite_buffer = gfx_create_empty_buffer(0, TILE_WIDTH / 2, TILE_HEIGHT / 2, TRUE);
    render_pattern_to_buffer_2(sprite_buffer->buffer, sprite_buffer->width, sprite_buffer->height);

    gfx_init_video();
    tileset_buffer = gfx_get_tileset_buffer();
    render_pattern_to_buffer_1(tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height);
    gfx_load_tileset();

    pos_x = 0;
    pos_y = 0;
    tile_offset_x = 0;
    tile_offset_y = 0;
    i = 0;
    while (1) {
        for(j = 0; j < testobj_count; j++) {
            cur_testobj = &testobj_list[j];
            gfx_draw_sprite_to_screen(sprite_buffer, 0, 0, (word) cur_testobj->xpos + (pos_x % TILE_WIDTH), (word) cur_testobj->ypos + (pos_y % TILE_HEIGHT), sprite_buffer->width, sprite_buffer->height);

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
        // if(pos_x / TILE_WIDTH != tile_offset_x || pos_y / TILE_HEIGHT != tile_offset_y) {
        //     tile_offset_x = pos_x / TILE_WIDTH;
        //     tile_offset_y = pos_y / TILE_HEIGHT;
        //     for(y = 0; y < 16; y++) {
        //         for(x = 0; x < 21; x++) {
        //             offset = (y + tile_offset_y) * 64 + x + tile_offset_x;
        //             gfx_set_tile(tilemap[offset], x, y);
        //         }
        //     }
        // }
        // gfx_set_scroll_offset(pos_x++ % TILE_WIDTH, pos_y % TILE_HEIGHT);
        pos_x++;
        // pos_y++;
        gfx_render_all();
        if(kbhit()) {
            ch = getch();
            if(ch == 27)
                break;
        }
    }

    vga_exit_modex();

    printf("total objects rendered: %d\n", testobj_count);

    return 0;
}


int main(int argc, char *argv[]) {
    char *a = argv[1];
    int num = atoi(a);

    if(argc >= 2)
        return test_scroll(num);
    else
        return test_scroll(256);

    // return test_vga();
}
