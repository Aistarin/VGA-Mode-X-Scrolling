#include "src/common.h"
#include "src/gfx/gfx.h"
#include "src/io/bitmap.h"
#include "src/io/file.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tile_width;                         // tile width
int tile_height;                        // tile height
int tileset_width;                      // tileset width
int tileset_height;                     // tileset height
int tileset_buffer_size;                // size of raw tileset data
int tileset_data_size;                  // size of total tileset data (in bytes)
gfx_tileset *tileset_data;              // struct that holds tileset data
byte *buffer;                           // bitmap that will hold data loaded from file

/* This builds a planar bitmask that checks whether or not a pixel is 0-valued (transparent).
 * NOTE: since this is 32-bit code, we can read in 4 bytes at a time when it
 * comes to implementing the masking in assembly. There's probably a better
 * way to build this, but this should suffice for now.
 */
void build_mask_bitmap() {
    word i, x, y, x_current = 0, y_current = 0;
    dword bitmask = 0, bit = 0, cur_offset = 0, buffer_offset;
    dword *mask_bitmap = (dword *) tileset_data->mask_bitmap;

    for(i = 0; i < 256; i++) {
        x_current = (i % 16) * tile_width;
        y_current = (i / 16) * tile_height;
        for(y = y_current; y < y_current + tile_height; y++) {
            for(x = x_current; x < x_current + tile_width; x += 4) {
                buffer_offset = (y * tileset_width ) + x;
                // each 4-bit nibble represents the value we set the SC_INDEX register
                // to select the planes to be latch-copied (which contain the
                // non-transparent pixels)
                bitmask |= buffer[buffer_offset] ? (1 << (31 - (bit + 3))) : 0;
                bitmask |= buffer[buffer_offset + 1] ? (1 << (31 - (bit + 2))) : 0;
                bitmask |= buffer[buffer_offset + 2] ? (1 << (31 - (bit + 1))) : 0;
                bitmask |= buffer[buffer_offset + 3] ? (1 << (31 - bit)) : 0;
                bit += 4;
                if(bit == 32) {
                    // circular rotate left 4 bits ahead of time to save us a ROL instruction
                    bitmask = (bitmask << 4 % 32) | (bitmask >> (32 - 4) % 32);
                    mask_bitmap[cur_offset++] = bitmask;
                    bitmask = 0;
                    bit = 0;
                }
            }
        }
    }
}

/* This converts the raw bitmap into planar format at the tile level in order
 * to allow tiles to either be drawn via VRAM-to-VRAM or as sprites
 */
void build_planar_bitmap() {
    word i, x, y, x_current = 0, y_current = 0;
    dword cur_offset = 0, buffer_offset;

    for(i = 0; i < 256; i++) {
        x_current = (i % 16) * tile_width;
        y_current = (i / 16) * tile_height;
        cur_offset = tile_width * tile_height * i;
        for(y = y_current; y < y_current + tile_height; y++) {
            for(x = x_current; x < x_current + tile_width; x += 4) {
                buffer_offset = (y * tileset_width ) + x;
                tileset_data->buffer[cur_offset] = buffer[buffer_offset];
                tileset_data->buffer[cur_offset + ((tile_width * tile_height) >> 2)] = buffer[buffer_offset + 1];
                tileset_data->buffer[cur_offset + (((tile_width * tile_height) >> 2) * 2)] = buffer[buffer_offset + 2];
                tileset_data->buffer[cur_offset + (((tile_width * tile_height) >> 2) * 3)] = buffer[buffer_offset + 3];
                cur_offset++;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    char *filename = argv[1];
    char *filename_to_save = argv[4];
    tile_width = atoi(argv[2]);
    tile_height = atoi(argv[3]);
    tileset_width = tile_width * 16;
    tileset_height = tile_height * 16;

    printf("Converting %s with %dx%d tiles to planar...\n", filename, tile_width, tile_height);

    tileset_buffer_size = tile_width * tile_height * 256;
    tileset_data_size = sizeof(struct gfx_tileset)  // data structure itself
        + (tileset_buffer_size * sizeof(byte))  // size of tileset data
        + ((tileset_buffer_size >> 3) * sizeof(byte))  // size of mask bitmap (1/8th of size of tileset data)
        + (sizeof(byte) * 256 * 3);  // size of palette data

    printf("Calculated tileset data size: %d bytes\n", tileset_data_size);

    tileset_data = (gfx_tileset *) malloc(tileset_data_size);

    tileset_data->tile_count = 256;
    tileset_data->tile_width = tile_width;
    tileset_data->tile_height = tile_height;
    tileset_data->tileset_width = tileset_width;
    tileset_data->tileset_height = tileset_height;
    tileset_data->buffer_size = tileset_buffer_size;
    tileset_data->mask_bitmap_size = tileset_buffer_size >> 3;

    /* tileset data is memory immediately after main struct */
    tileset_data->buffer = (byte *) tileset_data + sizeof(struct gfx_tileset);
    memset(tileset_data->buffer, 0, tileset_data->buffer_size);
    tileset_data->mask_bitmap = (byte *) tileset_data->buffer + tileset_data->buffer_size;
    memset(tileset_data->mask_bitmap, 0, tileset_data->mask_bitmap_size);
    tileset_data->palette = (byte *) tileset_data->mask_bitmap + tileset_data->mask_bitmap_size;
    memset(tileset_data->palette, 0, sizeof(byte) * 256 * 3);

    // allocate temp buffer and load raw tile bitmap into it;
    buffer = malloc(tileset_buffer_size * sizeof(byte));
    load_bmp_to_buffer(filename, buffer, (word) tileset_width, (word) tileset_height, tileset_data->palette, 0);

    // build the data
    build_mask_bitmap();
    build_planar_bitmap();

    // clear the pointers since they do not need to be saved
    tileset_data->buffer = NULL;
    tileset_data->mask_bitmap = NULL;
    tileset_data->palette = NULL;

    // save to file
    write_bytes_to_file(filename_to_save, (byte *) tileset_data, tileset_data_size);

    return 0;
}
