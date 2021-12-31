#include "vga.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159
#endif

unsigned int COSX[256];
unsigned int SINEY[256];

byte palette[256*3];

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
            if (!(j % TILE_SIZE && i % TILE_SIZE)) color = 0x40 + (((40 * (i + j)) / (height + width)));
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
            if (!(j % TILE_SIZE && i % TILE_SIZE)) color = 0x10 + (((16 * (i + j)) / (height + width)));
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

void load_bmp_to_buffer(char *file, byte *screen_buffer)
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

    for(index = (long) PAGE_HEIGHT; index > 0; index--) {
        for(x = 0; x < (int) PAGE_WIDTH; x++) {
            screen_buffer[PAGE_WIDTH * index + x] = (byte)fgetc(fp);
        }
    }

    fclose(fp);
}

int main(){
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
    // vga_set_palette(palette);

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
