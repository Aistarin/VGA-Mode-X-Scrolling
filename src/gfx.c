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

word *dirty_tile_buffer;                // buffer of all dirty tiles to be vram-to-vram blitted
word dirty_tile_count = 0;              // number of dirty tiles to be vram-to-vram blitted

word *dirty_sprite_tile_buffer;         // buffer of all dirty tiles to be main memory-to-vram blitted
word dirty_sprite_tile_count = 0;       // number of dirty tiles to be main memory-to-vram blitted

gfx_dirty_tile_offset *dirty_sprite_tile_offsets;       // buffer of dirty tile offsets
word dirty_sprite_tile_offset_count;                    // number of dirty tile offsets
byte *dirty_sprite_offset_counts;                       // number of dirty tile offsets per row

gfx_sprite_to_draw *sprites_to_draw;    // buffer of sprites to draw on screen
word sprites_to_draw_count = 0;         // number of sprites to draw

int frame_number = 0;

int plane_select[7] = {0,1,2,3,0,1,2};

struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height, bool is_planar) {
    struct gfx_buffer* empty_buffer;
    int buffer_size = (int) width * (int) height;

    /* align buffer size to multiple of int size */
    buffer_size = buffer_size + ((sizeof(unsigned int)) - buffer_size % sizeof(unsigned int));

    empty_buffer = malloc(sizeof(struct gfx_buffer) + (sizeof(byte) * buffer_size));

    empty_buffer->buffer_size = buffer_size;
    empty_buffer->is_planar = is_planar;
    empty_buffer->width = width;
    empty_buffer->height = height;

    /* bitmap buffer is memory immediately after main struct */
    empty_buffer->buffer = (byte *) empty_buffer + sizeof(struct gfx_buffer);

    memset(empty_buffer->buffer, 0, empty_buffer->buffer_size);

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
 * GRAPHICS API DEFINITIONS
 * 
 * These are the drawing routines that can be called
 * from elsewhere
 **/

/**
 * Handler for blitting entire main memory screen buffer into
 * the active page in VRAM
 **/
void gfx_blit_screen_buffer() {
    vga_blit_buffer_to_vram(
        gfx_screen_buffer->buffer,
        PAGE_WIDTH,
        PAGE_HEIGHT,
        0,
        0,
        0,
        current_render_page_offset,
        PAGE_WIDTH,
        PAGE_HEIGHT
    );
}

/**
 * Handler for mirroring the contents of one page to the other
 * via VRAM-to-VRAM copy
 **/
void gfx_mirror_page() {
    vga_blit_vram_to_vram(
        PAGE_WIDTH,
        PAGE_HEIGHT,
        0,
        current_render_page ? PAGE_HEIGHT : 0,
        PAGE_WIDTH,
        PAGE_HEIGHT
    );
}

void _gfx_draw_linear_bitmap_to_linear_bitmap(
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

void _gfx_draw_linear_bitmap_to_planar_bitmap(
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
    int dest_buffer_size = gfx_screen_buffer->buffer_size >> 2;
    byte *source_buffer = source_bitmap->buffer;
    word source_buffer_width = source_bitmap->width;
    word source_buffer_height = source_bitmap->height;
    int x, y, i, plane, offset, source_offset, plane_offset, plane_x_offset;
    int initial_plane = (dest_x % 4);
    int max_x = MIN(dest_buffer_width, dest_x + width);
    int max_y = MIN(dest_buffer_height, dest_y + height);
    int max_width = max_x - dest_x;
    int max_height = max_y - dest_y;

    for(i = 0; i < 4; i++) {
        plane = (initial_plane + i) % 4;
        plane_offset = dest_buffer_size * plane;
        for(y = 0; y < max_height; y++) {
            for(x = i; x < max_width; x += 4) {
                offset = ((dword) (dest_y + y) * PAGE_WIDTH + (dest_x + x)) >> 2;
                source_offset = source_buffer_width * (source_y + y) + (source_x + x);
                dest_buffer[plane_offset + offset] = source_buffer[source_offset];
            }
        }
    }
}

void _gfx_add_sprite_to_draw(gfx_buffer *sprite_buffer, word dest_x, word dest_y, word width, word height) {
    gfx_sprite_to_draw *cur_sprite = &sprites_to_draw[sprites_to_draw_count++];
    word max_width = MIN(PAGE_WIDTH, dest_x + width) - dest_x;
    word max_height = MIN(PAGE_HEIGHT, dest_y + height) - dest_y;

    cur_sprite->sprite_buffer = sprite_buffer;
    cur_sprite->dest_x = dest_x;
    cur_sprite->dest_y = dest_y;
    cur_sprite->width = max_width;
    cur_sprite->height = max_height;
}

void gfx_draw_bitmap_to_screen(gfx_buffer *bitmap, word source_x, word source_y, word dest_x, word dest_y, word width, word height) {
    byte x, y;
    byte tile_min_x = dest_x / TILE_WIDTH;
    byte tile_min_y = dest_y / TILE_HEIGHT;
    byte tile_max_x = MIN((dest_x + width - 1) / TILE_WIDTH, render_tile_width);
    byte tile_max_y = MIN((dest_y + height - 1) / TILE_HEIGHT, render_tile_height);
    int tile_offset;
    for(y = tile_min_y; y <= tile_max_y; y++)
        for(x = tile_min_x; x <= tile_max_x; x++){
            tile_offset = ((int) y * (int) render_tile_width) + (int) x;
            tile_index_main_states[tile_offset] &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2);
            tile_index_main_states[tile_offset] |= GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE;
        };
    // gfx_draw_planar_sprite_to_planar_screen(bitmap, dest_x, (word) dest_y);
    // _gfx_draw_linear_bitmap_to_planar_bitmap(bitmap, gfx_screen_buffer, source_x, source_y, dest_x, dest_y, width, height);
    _gfx_add_sprite_to_draw(bitmap, dest_x, dest_y, width, height);
}

void gfx_set_tile(byte tile, byte x, byte y) {
    int tile_offset = ((int) y * (int) render_tile_width) + (int) x;
    tile_index_main[tile_offset] = tile;

    /* indicate that a tile has been set + is dirty */
    tile_index_main_states[tile_offset] &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2);
    tile_index_main_states[tile_offset] |= GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_TILE;

    _gfx_draw_linear_bitmap_to_planar_bitmap(
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

void _gfx_draw_tile_to_planar_screen(byte tile, word tile_x, word tile_y) {
    byte *dest_buffer = gfx_screen_buffer->buffer;
    word dest_buffer_width = gfx_screen_buffer->width;
    word dest_buffer_height = gfx_screen_buffer->height;
    int dest_buffer_size = gfx_screen_buffer->buffer_size >> 2;
    byte *tile_buffer = &gfx_tileset_buffer->buffer[tile * TILE_WIDTH * TILE_HEIGHT];
    int dest_x = tile_x * TILE_WIDTH, dest_y = tile_y * TILE_HEIGHT;

    int x, y, i, plane, offset, source_offset, plane_offset, plane_x_offset;
    int initial_plane = (dest_x % 4);
    int max_x = MIN(dest_buffer_width, dest_x + TILE_WIDTH);
    int max_y = MIN(dest_buffer_height, dest_y + TILE_HEIGHT);
    int max_width = max_x - dest_x;
    int max_height = max_y - dest_y;

    for(i = 0; i < 4; i++) {
        plane = (initial_plane + i) % 4;
        plane_offset = dest_buffer_size * plane;
        for(y = 0; y < TILE_HEIGHT; y++) {
            /* TODO: for now we are pretending tile is already in planar format so we can
               perform entire width copy with a single memcpy to make more cache-friendly */
            for(x = i; x < TILE_WIDTH; x += 4) {
                offset = ((dword) (dest_y + y) * PAGE_WIDTH + (dest_x + x)) >> 2;
                source_offset = TILE_WIDTH * (dest_y + y) + (dest_x + x);
                dest_buffer[plane_offset + offset] = tile_buffer[source_offset];
            }
        }
    }
}

void gfx_draw_planar_sprite_to_planar_screen(gfx_buffer *sprite_bitmap, word dest_x, word dest_y) {
    byte *dest_buffer = gfx_screen_buffer->buffer;
    word dest_buffer_width = gfx_screen_buffer->width;
    word dest_buffer_height = gfx_screen_buffer->height;
    byte *sprite_buffer = sprite_bitmap->buffer;
    word sprite_buffer_width = sprite_bitmap->width;
    word sprite_buffer_height = sprite_bitmap->height;
    int dest_buffer_size = gfx_screen_buffer->buffer_size >> 2;

    int x, y, i, plane, offset, source_offset, plane_offset, plane_x_offset;
    int initial_plane = (dest_x % 4);
    int max_x = MIN(dest_buffer_width, dest_x + sprite_buffer_width);
    int max_y = MIN(dest_buffer_height, dest_y + sprite_buffer_height);
    int max_width = max_x - dest_x;
    int max_height = max_y - dest_y;

    for(i = 0; i < 4; i++) {
        plane = (initial_plane + i) % 4;
        plane_offset = dest_buffer_size * plane;
        for(y = 0; y < max_height; y++) {
            /* TODO: pretend sprite is already in planar format and perform
               entire width (planar_ copy with a single memcpy to make more cache-friendly */
            for(x = i; x < max_width; x += 4) {
                offset = ((dword) (dest_y + y) * PAGE_WIDTH + (dest_x + x)) >> 2;
                source_offset = sprite_buffer_width * y + x;
                dest_buffer[plane_offset + offset] = sprite_buffer[source_offset];
            }
        }
    }
}

void _gfx_clear_tile_at_index(word tile_offset) {
    word x = tile_offset % render_tile_width;
    word y = tile_offset / render_tile_width;
    byte tile = tile_index_main[tile_offset];

    _gfx_draw_linear_bitmap_to_planar_bitmap(
        gfx_tileset_buffer,
        gfx_screen_buffer,
        (tile % TILE_WIDTH) * TILE_WIDTH,
        (tile / TILE_HEIGHT) * TILE_HEIGHT,
        x * TILE_WIDTH,
        y * TILE_HEIGHT,
        TILE_WIDTH,
        TILE_HEIGHT
    );
    // _gfx_draw_tile_to_planar_screen(tile, x, y);
}

/**
 * Video initialization
 **/
void gfx_init_video() {
    vga_init_modex();
    vga_wait_for_retrace();
    vga_scroll_offset(0, 0);
    render_page_width = PAGE_WIDTH;
    render_page_height = PAGE_HEIGHT;
    gfx_screen_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, render_page_width, render_page_height, TRUE);

    /* tile atlas consists of 16x16 tiles, totalling 256 unique tiles */
    gfx_tileset_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, TILE_WIDTH * 16, TILE_HEIGHT * 16, FALSE);

    render_tile_width = render_page_width / TILE_WIDTH;
    render_tile_height = render_page_height / TILE_HEIGHT;

    // various tile buffer sizes determined by max number of tiles on-screen
    render_tile_count = (word) render_tile_width * (word) render_tile_height;

    tile_index_main = calloc(render_tile_count, sizeof(byte));
    tile_index_main_states = calloc(render_tile_count, sizeof(byte));
    dirty_tile_buffer = malloc(sizeof(word) * render_tile_count);
    dirty_sprite_tile_buffer = malloc(sizeof(word) * render_tile_count);
    dirty_sprite_tile_offsets = malloc(sizeof(gfx_dirty_tile_offset) * render_tile_count);
    dirty_sprite_offset_counts = malloc(render_tile_height);
    memset(dirty_sprite_offset_counts, 0, render_tile_height);

    sprites_to_draw = malloc(sizeof(gfx_sprite_to_draw) * 256);
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

void _gfx_blit_planar_screen() {
    int plane, offset = 0;
    int initial_offset = (current_render_page_offset * PAGE_WIDTH) >> 2;
    int buffer_size = gfx_screen_buffer->buffer_size >> 2;
    byte *screen_buffer = gfx_screen_buffer->buffer;
    byte *VGA = (byte *) 0xA0000;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);
        /* TODO: figure out how to blit one word/dword at a time! */
        memcpy(&VGA[initial_offset], &screen_buffer[offset], buffer_size);
        offset += buffer_size;
    }
}

void _gfx_blit_dirty_sprite_tiles_linear() {
    byte plane;
    word current_tile;
    dword x, y, i, vga_offset, dest_x, dest_y, source_x, source_y;
    byte *screen_buffer = gfx_screen_buffer->buffer;
    byte *VGA = (byte *) 0xA0000;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        for(i = 0; i < dirty_sprite_tile_count; i++) {
            current_tile = dirty_sprite_tile_buffer[i];
            /* TODO: this could be optimized better to use less variables */
            source_x = (word) (current_tile % render_tile_width) * TILE_WIDTH;
            source_y = (word) (current_tile / render_tile_width) * TILE_HEIGHT;
            dest_x = source_x;
            dest_y = source_y + current_render_page_offset;
            for(y = 0; y < TILE_HEIGHT; y++) {
                for(x = plane; x < TILE_WIDTH; x += 4) {
                    vga_offset = ((dword) (dest_y + y) * PAGE_WIDTH + (dest_x + x)) >> 2;
                    VGA[(word)vga_offset] = screen_buffer[PAGE_WIDTH * (source_y + y) + (source_x + x)];
                }
            }
        }
    }
}

void _gfx_blit_dirty_sprite_tiles_planar() {
    byte plane;
    dword x, y, i, plane_offset, current_row, row_count;
    dword initial_offset = (current_render_page_offset * PAGE_WIDTH) >> 2;
    byte *screen_buffer = gfx_screen_buffer->buffer;
    byte *VGA = (byte *) 0xA0000;
    gfx_dirty_tile_offset current_offset;
    dword row_offset;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        plane_offset = (gfx_screen_buffer->buffer_size >> 2) * plane;

        i = 0;
        current_row = 0;
        while(i < dirty_sprite_tile_offset_count) {
            row_count = dirty_sprite_offset_counts[current_row];
            row_offset = 0;
            for(y = 0; y < TILE_HEIGHT; y++) {
                for(x = i; x < i + row_count; x++) {
                    current_offset = dirty_sprite_tile_offsets[x];
                    memcpy(&VGA[initial_offset + current_offset.offset + row_offset], &screen_buffer[plane_offset + current_offset.offset + row_offset], current_offset.length);
                }
                row_offset += PAGE_WIDTH >> 2;
            }
            current_row++;
            i += row_count;
        }
    }
}

void _gfx_blit_dirty_tiles() {
    byte tile_index;
    word current_tile;
    dword x, y, i, vga_offset, tile_offset;
    byte *VGA = (byte *) 0xA0000;

    // pixel variable made volatile to prevent the compiler from
    // optimizing it out, since value is actually not used
    volatile byte pixel;

    outpw(SC_INDEX, ((word)0xff << 8) + MAP_MASK);      //select all planes
    outpw(GC_INDEX, 0x08);                              //set to or mode

    /* TODO: this could be optimized better to use less variables */
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

void gfx_blit_sprites() {
    byte plane, x_offset;
    dword i, x, y, sprite_width, sprite_height, sprite_offset, dest_x, dest_y, vga_offset;
    dword initial_offset = (current_render_page_offset * PAGE_WIDTH) >> 2;
    byte *sprite_buffer;
    byte *VGA = (byte *) 0xA0000;
    gfx_sprite_to_draw *cur_sprite;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        for(i = 0; i < sprites_to_draw_count; i++) {
            cur_sprite = &sprites_to_draw[i];
            sprite_width = cur_sprite->width >> 2;
            sprite_height = cur_sprite->height;
            sprite_buffer = cur_sprite->sprite_buffer->buffer;

            x_offset = plane - (cur_sprite->dest_x % 4);
            /* modulus again to account for overflow, can't do it on same line as above for some reason,
               it's likely getting optimized out (at least on Watcom) */
            x_offset = x_offset % 4;
            sprite_offset = (cur_sprite->sprite_buffer->buffer_size >> 2) * (dword) x_offset;
    
            dest_x = (cur_sprite->dest_x + x_offset) >> 2;
            dest_y = cur_sprite->dest_y + current_render_page_offset;

            for(x = 0; x < sprite_width; x++) {
                vga_offset = dest_y * (PAGE_WIDTH >> 2) + dest_x + x;
                for(y = 0; y < sprite_height; y++) {
                    // VGA[vga_offset] = screen_buffer[sprite_offset + sprite_height * x + y];
                    VGA[vga_offset] = sprite_buffer[sprite_offset++];
                    vga_offset += PAGE_WIDTH >> 2;
                }
            }
        }
    }
}

void _gfx_add_dirty_tile_offset(word tile_index, word tile_length) {
    gfx_dirty_tile_offset *cur_offset = &dirty_sprite_tile_offsets[dirty_sprite_tile_offset_count++];

    dirty_sprite_offset_counts[tile_index / render_tile_width]++;

    cur_offset->tile_index = tile_index;
    cur_offset->offset = ((((tile_index / render_tile_width) * PAGE_WIDTH * TILE_HEIGHT))
        + ((tile_index % render_tile_width) * TILE_WIDTH)) >> 2;
    cur_offset->length = (tile_length * TILE_WIDTH) >> 2;
}

/**
 * Main rendering call
 **/
void gfx_render_all() {
    int i, j;
    word current_tile_index;
    byte current_tile_state;
    word initial_dirty_sprite_tile_index;
    bool consecutive_dirty_sprite_tile = FALSE;
    dword current_dirty_offset;
    dword current_dirty_length;

    vga_wait_for_retrace();

    /* switch to offscreen rendering page */
    current_render_page_offset = (word) current_render_page * PAGE_HEIGHT;

    /* clear dirty sprite offset counts per row */
    memset(dirty_sprite_offset_counts, 0, render_tile_height);

    /* queue all of the dirty tiles in the main memory buffer */
    for(i = 0; i < render_tile_count; i++) {
        current_tile_state = tile_index_main_states[i];
        if(current_tile_state & (GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2)){
            if(current_tile_state & GFX_TILE_STATE_SPRITE) {
                /* tiles inserted front-to-back */
                dirty_sprite_tile_buffer[dirty_sprite_tile_count++] = i;

                if(!consecutive_dirty_sprite_tile) {
                    initial_dirty_sprite_tile_index = i;
                    consecutive_dirty_sprite_tile = TRUE;
                }

                if((i + 1) % render_tile_width == 0) {
                    consecutive_dirty_sprite_tile = FALSE;
                    _gfx_add_dirty_tile_offset(initial_dirty_sprite_tile_index, i - initial_dirty_sprite_tile_index + 1);
                }
            }
            else {
                if(consecutive_dirty_sprite_tile) {
                    consecutive_dirty_sprite_tile = FALSE;
                    _gfx_add_dirty_tile_offset(initial_dirty_sprite_tile_index, i - initial_dirty_sprite_tile_index);
                }

                if (current_tile_state & GFX_TILE_STATE_TILE) {
                    /* these tiles can be inserted back-to-front since buffer size
                    will never exceed the total tiles on screen */
                    dirty_tile_buffer[dirty_tile_count++] = i;
                }
            }
            /* decrement dirty persistent count by 1 */
            tile_index_main_states[i] = --current_tile_state;
        } else if(consecutive_dirty_sprite_tile) {
            consecutive_dirty_sprite_tile = FALSE;
            _gfx_add_dirty_tile_offset(initial_dirty_sprite_tile_index, i - initial_dirty_sprite_tile_index);
        }
    }

    // _gfx_blit_planar_screen();
    // if (dirty_sprite_tile_offset_count > 0)
    //     _gfx_blit_dirty_sprite_tiles_planar();

    if (dirty_tile_count > 0)
        _gfx_blit_dirty_tiles();

    if(sprites_to_draw_count > 0)
        gfx_blit_sprites();

    /* page flip + scrolling */
    vga_scroll_offset((word) view_scroll_x, current_render_page_offset + view_scroll_y);

    /* clear tiles on which sprites have been rendered to */
    /* TODO: looks like there is a bottleneck here! Perhaps it's also time to
       start storing + rendering sprites + tiles in planar format */
    for(i = 0; i < dirty_sprite_tile_count; i++) {
        current_tile_index = dirty_sprite_tile_buffer[i];
        // _gfx_clear_tile_at_index(current_tile_index);
        tile_index_main_states[current_tile_index] &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE);
        tile_index_main_states[current_tile_index] |= GFX_TILE_STATE_DIRTY_2;
    }

    /* clear dirty tile buffer once rendering has finished */
    dirty_tile_count = 0;
    dirty_sprite_tile_count = 0;
    dirty_sprite_tile_offset_count = 0;
    sprites_to_draw_count = 0;

    current_render_page = 1 - current_render_page;
    frame_number++;
}
