#include "gfx.h"
#include "vga.h"
#include <stdio.h>
#include <stdlib.h>

word render_page_width;             // rendering page width (visible + non-visible, in pixels)
word render_page_height;            // rendering page height (visible + non-visible, in pixels)
byte current_render_page;           // current page displayed

gfx_buffer_8bit *gfx_screen_buffer; // main memory representation of current screen

void gfx_init_video() {
    vga_init_modex();
    vga_scroll_offset(0, 0);
    render_page_width = PAGE_WIDTH;
    render_page_height = PAGE_HEIGHT;
    gfx_screen_buffer = gfx_create_empty_buffer_8bit(render_page_width, render_page_height);
};

struct gfx_buffer_8bit* gfx_create_empty_buffer_8bit(word width, word height) {
    int buffer_size;
    struct gfx_buffer_8bit* empty_buffer;

    buffer_size = ((int) width * (int) height) * sizeof(byte);
    empty_buffer = malloc(sizeof(gfx_buffer_8bit) + buffer_size);

    empty_buffer->buffer_size = buffer_size;
    empty_buffer->is_planar = 0;
    empty_buffer->width = width;
    empty_buffer->height = height;

    return empty_buffer;
};

byte* gfx_get_screen_buffer() {
    return gfx_screen_buffer->buffer;
};

/**
 * Handler for drawing a tile as a solid color
 * 
 * Command arguments:
 *  - arg0 (byte): index of color in palette
 *  - arg1 (byte): horizontal tile index
 *  - arg2 (byte): vertical tile index
 **/
void _gfx_draw_color(gfx_draw_command* command) {
    byte color = command->arg0;
    byte scr_tile_index_horz = command->arg1;
    byte scr_tile_index_vert = command->arg2;
};

/**
 * Handler for performing VRAM-to-VRAM blit of a given tile
 * 
 * Command arguments:
 *  - arg0 (byte): index of tile
 *  - arg1 (byte): horizontal tile index on screen
 *  - arg2 (byte): vertical tile index on screen
 **/
void _gfx_draw_tile(gfx_draw_command* command) {
    byte tile_index = command->arg0;
    byte scr_tile_index_horz = command->arg1;
    byte scr_tile_index_vert = command->arg2;
};

/**
 * Handler for performing main memory buffer-to-VRAM blit of
 * a tile (or series of tiles)
 * 
 * Can handle a maximum block copy of 16x16 tiles
 * 
 * Command arguments:
 *  - arg0 (byte): horizontal tile index in buffer
 *  - arg1 (byte): vertical tile index in buffer
 *  - arg2 (byte): dimensions (4-bit values)
 *      - high 4 bits: horizontal length (in tiles)
 *      - lower 4 bits: vertical length (in tiles)
 **/
void _gfx_blit_tiles(gfx_draw_command* command) {
    byte buf_tile_index_horz = command->arg0;
    byte buf_tile_index_vert = command->arg1;
    byte length_horz = command->arg2 & 0x0F;
    byte length_vert = command->arg2 >> 4;
};

/**
 * Handler for blitting entire main memory screen buffer into
 * the active page in VRAM
 * 
 * Command arguments:
 *  - None
 **/
void _gfx_blit_buffer() {
    word width = gfx_screen_buffer->width;
    word height = gfx_screen_buffer->height;
    byte *buffer = gfx_screen_buffer->buffer;
    // TODO: logic to draw on current page
    vga_blit_buffer_to_vram(buffer, width, height, 0, 0, 0, 0, width, height);  
};