#include "gfx.h"
#include "vga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

word render_page_width;                 // rendering page width (visible + non-visible, in pixels)
word render_page_height;                // rendering page height (visible + non-visible, in pixels)
byte render_tile_width;                 // rendering page width (in tiles)
byte render_tile_height;                // rendering page height (in tiles)
word render_tile_count;                 // total render tile count

byte current_render_page = 0;           // current page displayed
word current_render_page_offset = 0;    // current page offset in VRAM
byte view_scroll_x = 0;                 // horizontal scroll value
byte view_scroll_y = 0;                 // vertical scroll value

gfx_buffer *gfx_screen_buffer;          // main memory representation of current screen
gfx_buffer *gfx_tileset_buffer;         // main memory representation of current tileset

byte *tile_index_main;                  // tile index for main memory screen buffer
byte *tile_index_main_states;           // states for all main tile indexes
gfx_draw_command *gfx_display_list;     // buffer of all draw commands to be drawn in order
word gfx_command_count = 0;             // number of commands currently in display list
word gfx_command_count_max;             // maximum number of commands that can be put in the display list

word *dirty_tile_buffer;                // buffer of all dirty tiles to be vram-to-vram blitted
word dirty_tile_count = 0;              // number of dirty tiles to be vram-to-vram blitted
word *dirty_sprite_tile_buffer;         // buffer of all dirty tiles to be main memory-to-vram blitted
word dirty_sprite_tile_count = 0;            // number of dirty tiles to be main memory-to-vram blitted

int frame_number = 0;

struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height) {
    struct gfx_buffer* empty_buffer;
    int buffer_size = (int) width * (int) height;

    /* align buffer size to multiple of int size */
    buffer_size = buffer_size + ((sizeof(unsigned int)) - buffer_size % sizeof(unsigned int));

    empty_buffer = malloc(sizeof(struct gfx_buffer) + (sizeof(byte) * buffer_size));

    empty_buffer->buffer_size = buffer_size;
    empty_buffer->is_planar = 0;
    empty_buffer->width = width;
    empty_buffer->height = height;

    /* bitmap buffer is memory immediately after main struct */
    empty_buffer->buffer = (byte *) empty_buffer + sizeof(struct gfx_buffer);

    return empty_buffer;
}

/**
 * Returns pointer to the main memory buffer representation
 * of the screen
 **/
gfx_buffer* gfx_get_screen_buffer() {
    return gfx_screen_buffer;
}

/**
 * Returns pointer to the main memory buffer representation
 * of the currently loaded tileset
 **/
gfx_buffer* gfx_get_tileset_buffer() {
    return gfx_tileset_buffer;
}

/**
 * GRAPHICS COMMAND HANDLERS
 * 
 * These are the commands that map the graphics engine's
 * drawing commands to hardware-specific ones
 **/

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
}

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
    
    vga_blit_vram_to_vram(
        (tile_index % render_tile_width) * TILE_WIDTH,
        PAGE_HEIGHT * 2 + (tile_index / render_tile_width) * TILE_HEIGHT,
        scr_tile_index_horz * TILE_WIDTH,
        current_render_page_offset + scr_tile_index_vert * TILE_HEIGHT,
        TILE_WIDTH,
        TILE_HEIGHT
    );
}

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

    vga_blit_buffer_to_vram(
        gfx_screen_buffer->buffer,
        gfx_screen_buffer->width,
        gfx_screen_buffer->height,
        buf_tile_index_horz * TILE_WIDTH,
        buf_tile_index_vert * TILE_HEIGHT,
        buf_tile_index_horz * TILE_WIDTH,
        current_render_page_offset + buf_tile_index_vert * TILE_HEIGHT,
        TILE_WIDTH,
        TILE_HEIGHT
    );
}

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
    vga_blit_buffer_to_vram(buffer, width, height, 0, 0, 0, current_render_page_offset, width, height);
}

/**
 * Handler for mirroring the contents of one page to the other
 * via VRAM-to-VRAM copy
 * 
 * Command arguments:
 *  - None
 **/
void _gfx_mirror_page() {
    word mirror_render_page_offset = current_render_page ? PAGE_HEIGHT : 0;
    word width = gfx_screen_buffer->width;
    word height = gfx_screen_buffer->height;
    byte *buffer = gfx_screen_buffer->buffer;
    vga_blit_buffer_to_vram(buffer, width, height, 0, 0, 0, mirror_render_page_offset, width, height);
}

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
}

/**
 * GRAPHICS API DEFINITIONS
 * 
 * These are the drawing routines that can be called
 * from elsewhere
 **/

void gfx_blit_screen_buffer() {
    _gfx_add_command_to_display_list(GFX_BLIT_BUFFER, 0, 0 ,0);
}

void gfx_mirror_page() {
    _gfx_add_command_to_display_list(GFX_MIRROR_PAGE, 0, 0 ,0);
}

void _gfx_draw_bitmap_to_bitmap(
    gfx_buffer *source_bitmap,
    gfx_buffer *dest_bitmap,
    word source_x,
    word source_y,
    word dest_x,
    word dest_y,
    word width,
    word height
) {
    byte *dest_buffer = dest_bitmap->buffer;
    word dest_buffer_width = dest_bitmap->width;
    word dest_buffer_height = dest_bitmap->height;
    byte *source_buffer = source_bitmap->buffer;
    word source_buffer_width = source_bitmap->width;
    word source_buffer_height = source_bitmap->height;
    int i;
    int max_x = MIN(dest_buffer_width, dest_x + width);
    int max_y = MIN(dest_buffer_height, dest_y + height);
    int max_width = max_x - dest_x;
    int max_height = max_y - dest_y;

    for(i = 0; i < max_height; i++) {
        memcpy(&dest_buffer[dest_buffer_width * (dest_y + i) + dest_x], &source_buffer[source_buffer_width * (source_y + i) + source_x], sizeof(byte) * max_width);
    }
}

void gfx_draw_bitmap_to_screen(gfx_buffer *bitmap, word source_x, word source_y, word dest_x, word dest_y, word width, word height) {
    byte x, y;
    byte tile_min_x = dest_x / TILE_WIDTH;
    byte tile_min_y = dest_y / TILE_HEIGHT;
    byte tile_max_x = MIN((dest_x + width) / TILE_WIDTH, render_tile_width);
    byte tile_max_y = MIN((dest_y + height) / TILE_HEIGHT, render_tile_height);
    int tile_offset;
    for(y = tile_min_y; y <= tile_max_y; y++)
        for(x = tile_min_x; x <= tile_max_x; x++){
            tile_offset = ((int) y * (int) render_tile_width) + (int) x;
            tile_index_main_states[tile_offset] &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2);
            tile_index_main_states[tile_offset] |= GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE;
        };
    _gfx_draw_bitmap_to_bitmap(bitmap, gfx_screen_buffer, source_x, source_y, dest_x, dest_y, width, height);
}

void gfx_set_tile(byte tile, byte x, byte y) {
    int tile_offset = ((int) y * (int) render_tile_width) + (int) x;
    tile_index_main[tile_offset] = tile;

    /* indicate that a tile has been set + is dirty */
    tile_index_main_states[tile_offset] &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2);
    tile_index_main_states[tile_offset] |= GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_TILE;

    _gfx_draw_bitmap_to_bitmap(
        gfx_tileset_buffer,
        gfx_screen_buffer,
        (tile % TILE_WIDTH) * TILE_WIDTH,
        (tile / TILE_HEIGHT) * TILE_HEIGHT,
        x * TILE_WIDTH,
        y * TILE_HEIGHT,
        TILE_WIDTH,
        TILE_HEIGHT
    );
}

void _gfx_clear_tile_at_index(word tile_offset) {
    word x = tile_offset % render_tile_width;
    word y = tile_offset / render_tile_width;
    byte tile = tile_index_main[tile_offset];

    _gfx_draw_bitmap_to_bitmap(
        gfx_tileset_buffer,
        gfx_screen_buffer,
        (tile % TILE_WIDTH) * TILE_WIDTH,
        (tile / TILE_HEIGHT) * TILE_HEIGHT,
        x * TILE_WIDTH,
        y * TILE_HEIGHT,
        TILE_WIDTH,
        TILE_HEIGHT
    );
}

/**
 * Video initialization
 **/
void gfx_init_video() {
    vga_init_modex();
    vga_scroll_offset(0, 0);
    render_page_width = PAGE_WIDTH;
    render_page_height = PAGE_HEIGHT;
    gfx_screen_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, render_page_width, render_page_height);

    /* tile atlas consists of 16x16 tiles, totalling 256 unique tiles */
    gfx_tileset_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, TILE_WIDTH * 16, TILE_HEIGHT * 16);

    render_tile_width = render_page_width / TILE_WIDTH;
    render_tile_height = render_page_height / TILE_HEIGHT;
    render_tile_count = (word) render_tile_width * (word) render_tile_height;
    gfx_command_count_max = render_tile_count;
    // display list buffer size determined by max number of tiles on-screen
    gfx_display_list = malloc(sizeof(gfx_draw_command) * gfx_command_count_max);
    tile_index_main = calloc(render_tile_width * render_tile_height, sizeof(byte));
    tile_index_main_states = calloc(render_tile_width * render_tile_height, sizeof(byte));
    dirty_tile_buffer = malloc(sizeof(word) * render_tile_count);
    dirty_sprite_tile_buffer = malloc(sizeof(word) * render_tile_count);
}

/* this loads the tileset into the VRAM after the two pages */
void gfx_load_tileset() {
    word i;
    for(i = 0; i < 256; i++)
        vga_blit_buffer_to_vram(
            gfx_tileset_buffer->buffer,
            gfx_tileset_buffer->width,
            gfx_tileset_buffer->height,
            (i % TILE_WIDTH) * TILE_WIDTH,
            (i / TILE_WIDTH) * TILE_HEIGHT,
            (i % render_tile_width) * TILE_WIDTH,
            PAGE_HEIGHT * 2 + (i / render_tile_width) * TILE_HEIGHT,
            TILE_WIDTH,
            TILE_HEIGHT
        );
}

/**
 * Main rendering call
 **/
void gfx_render_all() {
    int i;
    byte x, y, main_tile_state, main_tile;
    gfx_draw_command *cur_command;

    /* switch to offscreen rendering page */
    current_render_page_offset = (word) current_render_page * PAGE_HEIGHT;

    for(i = 0; i < render_tile_width * render_tile_height; i++){
        main_tile = tile_index_main[i];
        main_tile_state = tile_index_main_states[i];
        if(main_tile_state & (GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2)){
            x = i % render_tile_width;
            y = i / render_tile_width;

            if(main_tile_state & GFX_TILE_STATE_SPRITE){
                /* blit tile to VRAM if graphics have been rendered to it */
                _gfx_add_command_to_display_list(GFX_BLIT_TILES, x, y, 0);

            }
            else if (main_tile_state & GFX_TILE_STATE_TILE){
                /* fast blit cached tile to page */
                _gfx_add_command_to_display_list(GFX_DRAW_TILE, main_tile, x, y);
            }

            /* decrement dirty persistent count by 1 */
            if(current_render_page){
                tile_index_main_states[i] = --main_tile_state;
            }
        }
    }

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
            case GFX_MIRROR_PAGE:
                _gfx_mirror_page(cur_command);
                break;
        }
    };

    /* clear display list once all commands have finished executing */
    gfx_command_count = 0;

    /* page flip + scrolling */
    vga_scroll_offset((word) view_scroll_x, current_render_page_offset + view_scroll_y);

    current_render_page = 1 - current_render_page;
    frame_number++;

    /* clear tiles on which sprites have been rendered to */
    for(i = 0; i < render_tile_width * render_tile_height; i++){
        main_tile_state = tile_index_main_states[i];
        if(main_tile_state & (GFX_TILE_STATE_SPRITE)) {
            _gfx_clear_tile_at_index(i);
            tile_index_main_states[i] &= ~GFX_TILE_STATE_SPRITE;
        }
    }
}

void _gfx_blit_dirty_tiles() {
    byte plane, tile_index;
    word current_tile;
    dword x, y, i, vga_offset, tile_offset, dest_x, dest_y, source_x, source_y;
    byte *screen_buffer = gfx_screen_buffer->buffer;
    byte *VGA = (byte *) 0xA0000;
    int line;

    // pixel variable made volatile to prevent the compiler from
    // optimizing it out, since value is actually not used
    volatile byte pixel;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        for(i = 0; i < dirty_sprite_tile_count; i++) {
            current_tile = dirty_sprite_tile_buffer[i];
            // x = (word) (current_tile % render_tile_width) * TILE_WIDTH;
            // y = (word) (current_tile / render_tile_width) * TILE_HEIGHT;
            // vga_offset = ((dword) (current_render_page_offset + y) * PAGE_WIDTH + x) >> 2;
            // tile_offset = PAGE_WIDTH * y + x;
            // for(y = 0; y < TILE_HEIGHT; y++) {
            //     for(x = plane; x < TILE_WIDTH; x += 4) {
            //         VGA[vga_offset] = screen_buffer[tile_offset + x];
            //     }
            //     tile_offset += PAGE_WIDTH;
            //     vga_offset += PAGE_WIDTH >> 2;
            // }
            source_x = (word) (current_tile % render_tile_width) * TILE_WIDTH;
            source_y = (word) (current_tile / render_tile_width) * TILE_HEIGHT;
            dest_x = source_x;
            dest_y = source_y + current_render_page_offset;
            for(y = 0; y < TILE_HEIGHT; y++) {
                for(x = plane; x < TILE_WIDTH; x += 4) {
                    vga_offset = ((dword) (dest_y + y) * PAGE_WIDTH + (dest_x + x)) >> 2;
                    VGA[(word)vga_offset]=screen_buffer[PAGE_WIDTH * (source_y + y) + (source_x + x)];
                }
            }
        }
    }

    outpw(SC_INDEX, ((word)0xff << 8) + MAP_MASK);      //select all planes
    outpw(GC_INDEX, 0x08);                              //set to or mode

    for(i = 0; i < dirty_tile_count; i++) {
        current_tile = dirty_tile_buffer[i];
        tile_index = tile_index_main[current_tile];
        x = (word) (current_tile % render_tile_width) * TILE_WIDTH;
        y = (word) (current_tile / render_tile_width) * TILE_HEIGHT;
        vga_offset = ((y + current_render_page_offset) * PAGE_WIDTH + x) >> 2;
        x = (word) (tile_index % render_tile_width) * TILE_WIDTH;
        y = (word) (tile_index / render_tile_width) * TILE_HEIGHT;
        tile_offset = ((y + PAGE_HEIGHT * 2) * PAGE_WIDTH + x) >> 2;
        for(y = 0; y < TILE_HEIGHT; y++) {
            for(x = 0; x < TILE_WIDTH >> 2; x++) {
                pixel = VGA[tile_offset + x];           //read pixel to load the latches
                VGA[vga_offset + x] = 0;                //write four pixels
            }
            tile_offset += PAGE_WIDTH >> 2;
            vga_offset += PAGE_WIDTH >> 2;
        }
    }

    outpw(GC_INDEX + 1, 0x0ff);
}

void gfx_render_all_test() {
    int i, j;
    byte current_tile, current_tile_state;

    /* switch to offscreen rendering page */
    current_render_page_offset = (word) current_render_page * PAGE_HEIGHT;

    /* queue all of the dirty tiles in the main memory buffer */
    for(i = 0; i < render_tile_count; i++) {
        current_tile_state = tile_index_main_states[i];
        if(current_tile_state & (GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2)){
            if(current_tile_state & GFX_TILE_STATE_SPRITE) {
                /* tiles inserted front-to-back */
                dirty_sprite_tile_buffer[dirty_sprite_tile_count++] = i;
            }
            else if (current_tile_state & GFX_TILE_STATE_TILE) {
                /* these tiles can be inserted back-to-front since buffer size
                   will never exceed the total tiles on screen */
                dirty_tile_buffer[dirty_tile_count++] = i;
            }

            /* decrement dirty persistent count by 1 */
            if(current_render_page)
                tile_index_main_states[i] = --current_tile_state;
        }
    }

    _gfx_blit_dirty_tiles();

    /* page flip + scrolling */
    vga_scroll_offset((word) view_scroll_x, current_render_page_offset + view_scroll_y);

    /* clear tiles on which sprites have been rendered to */
    for(i = 0; i < dirty_sprite_tile_count; i++)
        tile_index_main_states[dirty_sprite_tile_buffer[i]] &= ~GFX_TILE_STATE_SPRITE;

    /* clear dirty tile buffer once rendering has finished */
    dirty_tile_count = 0;
    dirty_sprite_tile_count = 0;

    current_render_page = 1 - current_render_page;
    frame_number++;
}
