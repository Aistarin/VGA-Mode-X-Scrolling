#include "gfx.h"
#include "vga.h"
#include <stdio.h>
#include <stdlib.h>

word render_page_width;                 // rendering page width (visible + non-visible, in pixels)
word render_page_height;                // rendering page height (visible + non-visible, in pixels)
byte render_tile_width;                 // rendering page width (in tiles)
byte render_tile_height;                // rendering page height (in tiles)

byte current_render_page;               // current page displayed
word current_render_page_offset;        // current page offset in VRAM
byte view_scroll_x;                     // horizontal scroll value
byte view_scroll_y;                     // vertical scroll value

gfx_buffer_8bit *gfx_screen_buffer;     // main memory representation of current screen
gfx_draw_command *gfx_display_list;     // buffer of all draw commands to be drawn in order
word gfx_command_count;                 // number of commands currently in display list
word gfx_command_count_max;             // maximum number of commands that can be put in the display list

void gfx_init_video() {
    vga_init_modex();
    vga_scroll_offset(0, 0);
    render_page_width = PAGE_WIDTH;
    render_page_height = PAGE_HEIGHT;
    gfx_screen_buffer = gfx_create_empty_buffer_8bit(render_page_width, render_page_height);

    render_tile_width = render_page_width / TILE_WIDTH;
    render_tile_height = render_page_height / TILE_HEIGHT;
    gfx_command_count_max = (word) render_tile_width * (word) render_tile_height;
    // display list buffer size determined by max number of tiles on-screen
    gfx_display_list = malloc(sizeof(gfx_draw_command) * gfx_command_count_max);
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

/**
 * Returns pointer to the main memory buffer representation
 * of the screen
 **/
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
    vga_blit_buffer_to_vram(buffer, width, height, 0, 0, 0, current_render_page_offset, width, height);
};

/**
 * Push drawing command to display list and increment count.
 **/
void _gfx_add_command_to_display_list(int command, byte arg0, byte arg1, byte arg2) {
    gfx_draw_command *cur_command;

    /* do nothing if display list is already full */
    if (gfx_command_count == gfx_command_count_max)
        return;

    cur_command = &gfx_display_list[gfx_command_count];

    cur_command->command = (byte) command;
    cur_command->arg0 = arg0;
    cur_command->arg1 = arg1;
    cur_command->arg2 = arg2;

    gfx_command_count++;
    printf("gfx_command_count: %d", gfx_command_count);
};

void gfx_blit_screen_buffer() {
    _gfx_add_command_to_display_list(GFX_BLIT_BUFFER, 0, 0 ,0);
};

/**
 * Main rendering call
 **/
void gfx_render_all() {
    int i;
    gfx_draw_command *cur_command;

    /* switch to offscreen rendering page */
    current_render_page = 1 - current_render_page;
    current_render_page_offset = (word) current_render_page * PAGE_HEIGHT;

    /* Loop through and execute all commands in display list */
    for (i = 0; i < gfx_command_count; i++) {
        cur_command = &gfx_display_list[i];
        switch((int) cur_command->command) {
            case GFX_DRAW_COLOR:
                _gfx_draw_color(cur_command);
                break;
            case GFX_DRAW_TILE:
                _gfx_draw_tile(cur_command);
                break;
            case GFX_BLIT_TILES:
                _gfx_blit_tiles(cur_command);
                break;
            case GFX_BLIT_BUFFER:
                _gfx_blit_buffer(cur_command);
                break;
        }
    };

    /* clear display list once all commands have finished executing */
    gfx_command_count = 0;

    /* page flip + scrolling */
    vga_scroll_offset((word) view_scroll_x, current_render_page_offset + view_scroll_y);
};
