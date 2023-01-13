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
word view_scroll_x = 0;                 // horizontal scroll value
word view_scroll_y = 0;                 // vertical scroll value

gfx_buffer *gfx_screen_buffer;          // main memory representation of current screen
gfx_buffer *gfx_tileset_buffer;         // main memory representation of current tileset

gfx_screen_state *screen_state_current; // pointer to current screen state
gfx_screen_state *screen_state_page_0;  // screen state for VRAM page 0
gfx_screen_state *screen_state_page_1;  // screen state for VRAM page 1

gfx_sprite_to_draw *sprites_to_draw;
word sprites_to_draw_count;

gfx_tilemap *screen_tilemap;            // pointer to tilemap

int frame_number = 0;

struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height, bool is_planar) {
    struct gfx_buffer* empty_buffer;
    int buffer_size = (int) width * (int) height;

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

struct gfx_screen_state* _init_screen_state(byte horz_tiles, byte vert_tiles) {
    struct gfx_screen_state* screen_state;
    word tile_count = (word) horz_tiles * (word) vert_tiles;
    dword bytes_to_allocate = sizeof(struct gfx_screen_state)
        + sizeof(gfx_tile_state) * tile_count               // memory for tile index
        + sizeof(word) * tile_count                         // memory for tile to clear
        + sizeof(word) * tile_count                         // memory for tiles to update
        + sizeof(struct gfx_sprite_to_draw) * 256;          // memory for sprites to draw

    screen_state = (gfx_screen_state *) malloc(bytes_to_allocate);

    screen_state->tile_count = tile_count;
    screen_state->horz_tiles = horz_tiles;
    screen_state->vert_tiles = vert_tiles;
    screen_state->tiles_to_clear_count = 0;
    screen_state->tiles_to_update_count = 0;

    /* set pointers to contiguous memory locations */
    screen_state->tile_index = (gfx_tile_state *) (screen_state + sizeof(struct gfx_screen_state));
    memset(screen_state->tile_index, 0, sizeof(struct gfx_tile_state) * tile_count);
    screen_state->tiles_to_clear = (word *) (screen_state->tile_index + sizeof(struct gfx_sprite_to_draw) * 256);
    screen_state->tiles_to_update = (word *) (screen_state->tiles_to_clear + sizeof(word) * tile_count);

    return screen_state;
}

struct gfx_tilemap* _init_tilemap(byte horz_tiles, byte vert_tiles) {
    struct gfx_tilemap* tilemap;
    word tile_count = (word) horz_tiles * (word) vert_tiles;
    dword bytes_to_allocate = sizeof(struct gfx_tilemap)
        + sizeof(gfx_tilemap) * tile_count;                 // memory for tilemap buffer

    tilemap = (gfx_tilemap *) malloc(bytes_to_allocate);

    tilemap->tile_count = tile_count;
    tilemap->horz_tiles = horz_tiles;
    tilemap->vert_tiles = vert_tiles;
    tilemap->horz_offset = 0;
    tilemap->vert_offset = 0;

    /* set pointers to contiguous memory locations */
    tilemap->buffer = (byte *) (tilemap + sizeof(struct gfx_tilemap));
    memset(tilemap->buffer, 0, sizeof(byte) * tile_count);

    return tilemap;
}

void _set_tile_states(gfx_screen_state *screen_state, byte tile_state, bool clear, byte x, byte y, byte max_x, byte max_y) {
    byte i, j;
    word tile_offset;
    for(j = y; j <= max_y; j++)
        for(i = x; i <= max_x; i++){
            tile_offset = ((word) j * (word) screen_state->horz_tiles) + (word) i;
            if(clear) 
                screen_state->tile_index[tile_offset].state &= ~(tile_state);
            else
                screen_state->tile_index[tile_offset].state |= tile_state;
        };
}

void _gfx_add_sprite_to_draw(gfx_screen_state *screen_state, gfx_buffer *sprite_buffer, word dest_x, word dest_y, word width, word height) {
    gfx_sprite_to_draw *cur_sprite = &sprites_to_draw[sprites_to_draw_count++];
    word max_width = MIN(PAGE_WIDTH, dest_x + width) - dest_x;
    word max_height = MIN(PAGE_HEIGHT, dest_y + height) - dest_y;

    cur_sprite->sprite_buffer = sprite_buffer;
    cur_sprite->dest_x = dest_x;
    cur_sprite->dest_y = dest_y;
    cur_sprite->width = max_width;
    cur_sprite->height = max_height;
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
 * Returns pointer to tilemap
 **/
gfx_tilemap* gfx_get_tilemap_buffer() {
    return screen_tilemap;
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
        screen_state_current->current_render_page_offset,  /* TODO: change to absolute offset */
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

void gfx_load_linear_bitmap_to_planar_bitmap(byte *source_bitmap, byte *dest_bitmap, word width, word height) {
    byte plane;
    word x, y;
    dword offset;

    for(plane = 0; plane < 4; plane++){
        offset = ((width * height) >> 2) * plane;
        for(x = plane; x < width; x += 4) {
            for(y = 0; y < height; y++) {
                dest_bitmap[offset++] = source_bitmap[y * width + x];
            }
        }
    }
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

void gfx_draw_sprite_to_screen(gfx_buffer *bitmap, word source_x, word source_y, word dest_x, word dest_y, word width, word height) {
    word tile_x = dest_x / TILE_WIDTH;
    word tile_y = dest_y / TILE_HEIGHT;
    word tile_max_x = ((dest_x + width - 1) / TILE_WIDTH);
    word tile_max_y = ((dest_y + height - 1) / TILE_HEIGHT);
    _set_tile_states(
        screen_state_current,
        GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE,
        FALSE,
        tile_x,
        tile_y,
        tile_max_x,
        tile_max_y
    );
    _gfx_add_sprite_to_draw(screen_state_current, bitmap, dest_x, dest_y, width, height);
}

void _set_tile_for_screen_state(gfx_screen_state *screen_state, byte tile, byte x, byte y, bool force) {
    word tile_offset = ((word) y * (word) render_tile_width) + (word) x;
    gfx_tile_state *tile_state = &screen_state->tile_index[tile_offset];

    /* only mark tile as dirty if the tile has changed */
    if(!(tile_state->state & GFX_TILE_STATE_TILE) || tile_state->tile != tile || force) {
        tile_state->tile = tile;
        _set_tile_states(screen_state, GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_TILE, FALSE, x, y, x, y);
    }
}

void gfx_set_tile(byte tile, byte x, byte y) {
    _set_tile_for_screen_state(screen_state_page_0, tile, x, y, FALSE);
    _set_tile_for_screen_state(screen_state_page_1, tile, x, y, FALSE);
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

/**
 * Video initialization
 **/
void gfx_init_video() {
    word i;

    vga_init_modex();
    vga_wait_for_retrace();
    vga_scroll_offset(0, 0);
    render_page_width = PAGE_WIDTH;
    render_page_height = PAGE_HEIGHT;
    // gfx_screen_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, render_page_width, render_page_height, TRUE);

    /* tile atlas consists of 16x16 tiles, totalling 256 unique tiles */
    gfx_tileset_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, TILE_WIDTH * 16, TILE_HEIGHT * 16, FALSE);

    render_tile_width = render_page_width / TILE_WIDTH;
    /* render tile height is screen hight in tiles + 1 tile*/
    render_tile_height = render_page_height / TILE_HEIGHT - 1;

    // various tile buffer sizes determined by max number of tiles on-screen
    render_tile_count = (word) render_tile_width * (word) render_tile_height;

    screen_state_page_0 = _init_screen_state(render_tile_width, render_tile_height);
    screen_state_page_1 = _init_screen_state(render_tile_width, render_tile_height);
    screen_state_page_0->initial_render_page_offset = 0;
    screen_state_page_0->current_render_page_offset = 0;
    screen_state_page_1->initial_render_page_offset = (PAGE_WIDTH * PAGE_HEIGHT) >> 2;
    screen_state_page_1->current_render_page_offset = (PAGE_WIDTH * PAGE_HEIGHT) >> 2;

    /* set current rendering page to the offscreen one */
    screen_state_current = screen_state_page_1;
    current_render_page = 1;

    sprites_to_draw = malloc(sizeof(struct gfx_sprite_to_draw) * 256);

    screen_tilemap = _init_tilemap(255, 255);
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
    int initial_offset = screen_state_current->current_render_page_offset;
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

void _gfx_blit_dirty_tiles(bool sprite_tiles) {
    word *tiles_to_blit = sprite_tiles ? screen_state_current->tiles_to_clear : screen_state_current->tiles_to_update;
    word tiles_to_blit_count = sprite_tiles ? screen_state_current->tiles_to_clear_count : screen_state_current->tiles_to_update_count;
    byte tile_index;
    word current_tile;
    dword x, y, i, vga_offset, tile_offset, initial_offset = screen_state_current->current_render_page_offset;
    byte *VGA = (byte *) 0xA0000;

    /* pixel variable made volatile to prevent the compiler from
       optimizing it out, since value is not actually used */
    volatile byte pixel;

    outpw(SC_INDEX, ((word)0xff << 8) + MAP_MASK);      //select all planes
    outpw(GC_INDEX, 0x08);                              //set to OR mode

    /* TODO: this could be optimized better to use less variables */
    /* TODO: should probably just read the tile_index for the page directly */
    for(i = 0; i < tiles_to_blit_count; i++) {
        current_tile = tiles_to_blit[i];
        tile_index = screen_state_current->tile_index[current_tile].tile;
        x = (word) (current_tile % render_tile_width) * TILE_WIDTH;
        y = (word) (current_tile / render_tile_width) * TILE_HEIGHT;
        vga_offset = ((y * PAGE_WIDTH + x) >> 2) + initial_offset;
        x = (word) (tile_index % render_tile_width) * TILE_WIDTH;
        y = (word) (tile_index / render_tile_width) * TILE_HEIGHT;
        tile_offset = ((y + PAGE_HEIGHT * 2) * PAGE_WIDTH + x) >> 2;
        for(y = 0; y < TILE_HEIGHT; y++) {
            for(x = 0; x < TILE_WIDTH >> 2; x++) {
                pixel = VGA[tile_offset + x];           //read pixel to load all the latches
                VGA[vga_offset + x] = 0;                //write four pixels in parallel
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
    dword initial_offset = screen_state_current->current_render_page_offset;
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
            dest_y = cur_sprite->dest_y;

            for(x = 0; x < sprite_width; x++) {
                vga_offset = dest_y * (PAGE_WIDTH >> 2) + dest_x + x + initial_offset;
                for(y = 0; y < sprite_height; y++) {
                    VGA[vga_offset] = sprite_buffer[sprite_offset++];
                    vga_offset += PAGE_WIDTH >> 2;
                }
            }
        }
    }
}

void _scroll_screen_tiles(gfx_screen_state* screen_state, gfx_tilemap* tilemap, int horz_tile_offset, int vert_tile_offset) {
    byte x, y;
    word i;
    word tile_index_offset = 0;
    word tilemap_offset;
    word tile_row_size = (PAGE_WIDTH >> 2) * TILE_HEIGHT;
    bool scroll_up = vert_tile_offset < 0;
    bool scroll_down = vert_tile_offset > 0;
    bool scroll_left = horz_tile_offset < 0;
    bool scroll_right = horz_tile_offset > 0;
    byte *tilemap_buffer = tilemap->buffer;
    int new_render_page_offset;
    bool bottom_of_page = FALSE;
    bool top_of_page = FALSE;

    new_render_page_offset = (int) screen_state->current_render_page_offset + (horz_tile_offset * TILE_WIDTH >> 2) + (vert_tile_offset * (((render_tile_width * TILE_WIDTH) >> 2) * TILE_HEIGHT));

    /* bounds checks to determine whether or not new offset will
       push rendering past bottom or top of its page*/
    bottom_of_page = new_render_page_offset - (int) screen_state->initial_render_page_offset > (int) tile_row_size;
    top_of_page = new_render_page_offset < (int) screen_state->initial_render_page_offset;

    // TODO: perhaps put top/bottom of page memcpys here so that they get applied to all scroll directions
    if(bottom_of_page) {
        new_render_page_offset -= tile_row_size;
    } else if (top_of_page) {
        new_render_page_offset += tile_row_size;
    }

    //printf("current_render_page: %d, new_render_page_offset: %d\n", current_render_page, new_render_page_offset);

    /* set horizontal and vertical offset for screen state */
    screen_state->current_render_page_offset = new_render_page_offset;

    if(scroll_left || scroll_right) {
        tilemap_offset = tilemap->vert_offset * tilemap->vert_tiles + tilemap->horz_offset + (scroll_left ? -1 : render_tile_width);
        for(y = 0; y < render_tile_height; y++) {        
            if(scroll_left) {
                /* TODO: handle case where offset goes beyond the limit */
                /* shift all tiles to the right */
                for(x = render_tile_width - 1; x > 0; x--) {
                    screen_state->tile_index[tile_index_offset + x] = screen_state->tile_index[tile_index_offset + x - 1];
                }
                /* populate incoming tiles on the left */
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset], 0, y, TRUE);
            } else {
                /* TODO: handle case where offset goes below zero and wraps around */
                /* shift all tiles to the left */
                for(x = 1; x < render_tile_width; x++) {
                    screen_state->tile_index[tile_index_offset + x - 1] = screen_state->tile_index[tile_index_offset + x];
                }
                /* populate incoming tiles on the right */
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset], render_tile_width - 1, y, TRUE);
            }
            tile_index_offset += render_tile_width;
            tilemap_offset += tilemap->horz_tiles;
        }

        /* increment or decrement all tile indexes to be cleared */
            /* increment or decrement all tile indexes to be cleared */
        if (!(bottom_of_page || top_of_page)) {
            for(i = 0; i < screen_state->tiles_to_clear_count; i++) {
                screen_state->tiles_to_clear[i] += scroll_left ? 1 : -1;
                /* modulus to account for overflow */
                screen_state->tiles_to_clear[i] %= screen_state->tile_count;
            }
        }
    }

    if(scroll_up || scroll_down) {
        if(scroll_up) {
            tilemap_offset = (tilemap->vert_offset - 1) * tilemap->horz_tiles + tilemap->horz_offset;
            if(top_of_page) {
                for(y = 1; y <= render_tile_height - 1; y++) {
                    memcpy(
                        &screen_state->tile_index[(y - 1) * render_tile_width],
                        &screen_state->tile_index[y * render_tile_width],
                        sizeof(gfx_tile_state) * render_tile_width
                    );
                    for(x = 0; x < render_tile_width; x++)
                        _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, y, TRUE);
                    tilemap_offset += tilemap->horz_tiles;
                }

                for(x = 0; x < render_tile_width; x++)
                    _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, render_tile_height - 1, TRUE);
            } else {
                for(y = render_tile_height - 1; y > 0; y--)
                    memcpy(
                        &screen_state->tile_index[y * render_tile_width],
                        &screen_state->tile_index[(y - 1) * render_tile_width],
                        sizeof(gfx_tile_state) * render_tile_width
                    );
                for(x = 0; x < render_tile_width; x++)
                    _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, 0, TRUE);
            }
        } else if(scroll_down) {
            tilemap_offset = (tilemap->vert_offset + render_tile_height) * tilemap->horz_tiles + tilemap->horz_offset;
            if(bottom_of_page) {
                for(y = render_tile_height - 1; y > 0; y--) {
                    memcpy(
                        &screen_state->tile_index[y * render_tile_width],
                        &screen_state->tile_index[(y - 1) * render_tile_width],
                        sizeof(gfx_tile_state) * render_tile_width
                    );
                    for(x = 0; x < render_tile_width; x++)
                        _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, y, FALSE);
                    tilemap_offset -= tilemap->horz_tiles;
                }

                for(x = 0; x < render_tile_width; x++)
                    _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, 0, TRUE);
            } else {
                for(y = 1; y <= render_tile_height - 1; y++)
                    memcpy(
                        &screen_state->tile_index[(y - 1) * render_tile_width],
                        &screen_state->tile_index[y * render_tile_width],
                        sizeof(gfx_tile_state) * render_tile_width
                    );

                for(x = 0; x < render_tile_width; x++)
                    _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, render_tile_height - 1, TRUE);
            }
        }

        /* increment or decrement all tile indexes to be cleared */
        if (!(bottom_of_page || top_of_page)) {
            for(i = 0; i < screen_state->tiles_to_clear_count; i++) {
                screen_state->tiles_to_clear[i] += scroll_up ? render_tile_width * -1 : render_tile_width;
                /* modulus to account for overflow */
                screen_state->tiles_to_clear[i] %= screen_state->tile_count;
            }
        }
    }
}

void _scroll_screen_tile_orig(gfx_screen_state* screen_state, gfx_tilemap* tilemap, int horz_tile_offset, int vert_tile_offset) {
    byte x, y;
    word i;
    word tile_index_offset = 0;
    word tilemap_offset;
    bool scroll_up = vert_tile_offset < 0;
    bool scroll_down = vert_tile_offset > 0;
    bool scroll_left = horz_tile_offset < 0;
    bool scroll_right = horz_tile_offset > 0;
    byte *tilemap_buffer = tilemap->buffer;

    /* set horizontal and vertical offset for screen state */
    screen_state->current_render_page_offset += (horz_tile_offset * TILE_WIDTH >> 2) + (vert_tile_offset * (((render_tile_width * TILE_WIDTH) >> 2) * TILE_HEIGHT));

    if(scroll_left || scroll_right) {
        tilemap_offset = tilemap->vert_offset * tilemap->vert_tiles + tilemap->horz_offset + (scroll_left ? -1 : render_tile_width);
        for(y = 0; y < render_tile_height; y++) {        
            if(scroll_left) {
                /* TODO: handle case where offset goes beyond the limit */
                /* shift all tiles to the right */
                for(x = render_tile_width - 1; x > 0; x--) {
                    screen_state->tile_index[tile_index_offset + x] = screen_state->tile_index[tile_index_offset + x - 1];
                }
                /* populate incoming tiles on the left */
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset], 0, y, TRUE);
            } else {
                /* TODO: handle case where offset goes below zero and wraps around */
                /* shift all tiles to the left */
                for(x = 1; x < render_tile_width; x++) {
                    screen_state->tile_index[tile_index_offset + x - 1] = screen_state->tile_index[tile_index_offset + x];
                }
                /* populate incoming tiles on the right */
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset], render_tile_width - 1, y, TRUE);
            }
            tile_index_offset += render_tile_width;
            tilemap_offset += tilemap->horz_tiles;
        }
        /* increment or decrement all tile indexes to be cleared */
        for(i = 0; i < screen_state->tiles_to_clear_count; i++) {
            screen_state->tiles_to_clear[i] += scroll_left ? 1 : -1;
            /* modulus to account for overflow */
            screen_state->tiles_to_clear[i] %= screen_state->tile_count;
        }
    }

    if(scroll_up || scroll_down) {
        tilemap_offset = (tilemap->vert_offset + (scroll_up ? -1 : (render_tile_height))) * tilemap->horz_tiles + tilemap->horz_offset;
        if(scroll_up) {
            /* TODO: handle case where offset goes below zero and wraps around */
            for(y = render_tile_height - 1; y > 0; y--)
                memcpy(
                    &screen_state->tile_index[y * render_tile_width],
                    &screen_state->tile_index[(y - 1) * render_tile_width],
                    sizeof(gfx_tile_state) * render_tile_width
                );
            for(x = 0; x < render_tile_width; x++)
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, 0, TRUE);
        } else {
            /* TODO: handle case where offset goes beyond the limit */
            for(y = 1; y <= render_tile_height - 1; y++)
                memcpy(
                    &screen_state->tile_index[(y - 1) * render_tile_width],
                    &screen_state->tile_index[y * render_tile_width],
                    sizeof(gfx_tile_state) * render_tile_width
                );
            for(x = 0; x < render_tile_width; x++)
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, render_tile_height - 1, TRUE);
        }

        /* increment or decrement all tile indexes to be cleared */
        for(i = 0; i < screen_state->tiles_to_clear_count; i++) {
            screen_state->tiles_to_clear[i] += scroll_up ? render_tile_width * -1 : render_tile_width;
            /* modulus to account for overflow */
            screen_state->tiles_to_clear[i] %= screen_state->tile_count;
        }
    }
}

void _wrap_screen_offset(gfx_screen_state* screen_state) {
    word i;
    word row_size = (PAGE_WIDTH >> 2) * TILE_HEIGHT;
    // if offset goes beyond the end of the page, then mark all tiles as dirty, then jump
    // back up to the top of the page
    if(screen_state->current_render_page_offset - screen_state->initial_render_page_offset >= row_size) {
        // screen_state->current_render_page_offset -= row_size;
        screen_state->current_render_page_offset = screen_state->initial_render_page_offset;
        screen_state->tiles_to_update_count = screen_state->tile_count;
        for(i = 0; i < screen_state->tile_count; i++) {
            screen_state->tiles_to_update[i] = i;
        }
    }
}

void gfx_set_scroll_offset(word x_offset, word y_offset) {
    int horz_tile_offset = (x_offset / TILE_WIDTH) - (view_scroll_x / TILE_WIDTH);
    int vert_tile_offset = (y_offset / TILE_HEIGHT) - (view_scroll_y / TILE_HEIGHT);

    if(horz_tile_offset != 0 || vert_tile_offset != 0) {
        _scroll_screen_tile_orig(screen_state_page_0, screen_tilemap, horz_tile_offset, vert_tile_offset);
        _scroll_screen_tile_orig(screen_state_page_1, screen_tilemap, horz_tile_offset, vert_tile_offset);
        _wrap_screen_offset(screen_state_page_0);
        _wrap_screen_offset(screen_state_page_1);
        screen_tilemap->horz_offset += horz_tile_offset;
        screen_tilemap->vert_offset += vert_tile_offset;
    }

    /* modulus to account for overflow */
    // TODO: may not be necessary
    screen_tilemap->horz_offset %= 256;
    screen_tilemap->vert_offset %= 256;

    view_scroll_x = x_offset;
    view_scroll_y = y_offset;
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
    word vga_offset_x;
    word vga_offset_y;

    vga_wait_for_retrace();

    /* clear tiles that have had sprites rendered to them previously */
    if (screen_state_current->tiles_to_clear_count > 0)
        _gfx_blit_dirty_tiles(TRUE);
        screen_state_current->tiles_to_clear_count = 0;

    /* queue all of the dirty tiles in the main memory buffer */
    for(i = 0; i < render_tile_count; i++) {
        current_tile_state = screen_state_current->tile_index[i].state;
        if(current_tile_state & (GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2)){
            if(current_tile_state & GFX_TILE_STATE_SPRITE) {
                /* tiles inserted front-to-back */
                screen_state_current->tiles_to_clear[screen_state_current->tiles_to_clear_count++] = i;
            }
            else {
                if (current_tile_state & GFX_TILE_STATE_TILE) {
                    /* these tiles can be inserted back-to-front since buffer size
                    will never exceed the total tiles on screen */
                    screen_state_current->tiles_to_update[screen_state_current->tiles_to_update_count++] = i;
                }
            }
            /* clear state */
            screen_state_current->tile_index[i].state &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE);
        }
    }

    if (screen_state_current->tiles_to_update_count > 0)
        _gfx_blit_dirty_tiles(FALSE);

    if(sprites_to_draw_count > 0)
        gfx_blit_sprites();

    /* page flip + scrolling */
    vga_offset_x = ((view_scroll_x % TILE_WIDTH) >> 2);
    vga_offset_y = ((view_scroll_y % TILE_HEIGHT) * TILE_WIDTH * render_tile_width) >> 2;
    vga_set_offset(screen_state_current->current_render_page_offset + vga_offset_x + vga_offset_y);
    vga_set_horizontal_pan((byte) view_scroll_x);

    /* clear buffers once rendering has finished */
    screen_state_current->tiles_to_update_count = 0;
    sprites_to_draw_count = 0;

    current_render_page = 1 - current_render_page;
    screen_state_current = current_render_page ? screen_state_page_1 : screen_state_page_0;
    frame_number++;
}
