# include "pattern.h"

/*
 * Renders greyscale gradient tiles with rainbow borders
 */
void render_pattern_to_buffer_1(byte *screen_buffer, word tile_width, word tile_height, word width, word height) {
    word i, j, color;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            if (!(j % tile_height && i % tile_width)) color = 0x40 + (((40 * (i + j)) / (height + width)));
            else color = 0x10 + (((16 * (i + j)) / (height + width)));
            // else color = 0;
            screen_buffer[width * j + i] = color;
        }
    }
}

/*
 * Renders rainbow gradient tiles with greyscale borders
 */
void render_pattern_to_buffer_2(byte *screen_buffer, word tile_width, word tile_height, word width, word height) {
    word i, j, color;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            if (!(j % tile_height && i % tile_width)) color = 0x10 + (((16 * (i + j)) / (height + width)));
            else color = 0x40 + (((40 * (i + j)) / (height + width)));
            // else color = 0;
            screen_buffer[width * j + i] = color;
        }
    }
}

/*
 * Renders solid colored tiles that correspond to loaded palette
 */
void render_pattern_to_buffer_3(byte *screen_buffer, word tile_width, word tile_height, word width, word height) {
    word i, j;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            screen_buffer[width * j + i] = ((j / tile_height) * 16) + ((i / tile_width) % 16);
        }
    }
}
