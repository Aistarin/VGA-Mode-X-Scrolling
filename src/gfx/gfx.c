#include "gfx.h"
#include "vga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

byte *VGA = (byte *) VGA_START;

word render_page_width;                 // rendering page width (visible + non-visible, in pixels)
word render_page_height;                // rendering page height (visible + non-visible, in pixels)
byte render_tile_width;                 // rendering page width (in tiles)
byte render_tile_height;                // rendering page height (in tiles)
word render_tile_count;                 // total render tile count

byte current_render_page = 0;           // current page displayed
int view_scroll_x = 0;                  // horizontal scroll value
int view_scroll_y = 0;                  // vertical scroll value

gfx_buffer *gfx_screen_buffer;          // main memory representation of current screen
gfx_buffer *gfx_tileset_buffer;         // main memory representation of current tileset

gfx_screen_state *screen_state_current; // pointer to current screen state
gfx_screen_state *screen_state_page_0;  // screen state for VRAM page 0
gfx_screen_state *screen_state_page_1;  // screen state for VRAM page 1

gfx_sprite_to_draw *sprites_to_draw;
word sprites_to_draw_count;

gfx_tilemap *screen_tilemap;            // pointer to tilemap

byte x_offset_clipping_left[256][4];
byte x_offset_clipping_right[256][4];
byte plane_offset_clipping_left[256][4];
byte plane_offset_clipping_right[256][4];

int frame_number = 0;

/*
 * NOTE: I am certain there is a better way to calculate these values
 * on-the-fly, but this should suffice in the meantime until I can
 * figure it out
 */
void _init_clipping_tables() {
    int i;
    dword plane_vals_left = 0x03020100;
    dword plane_vals_right = 0x03020100;
    byte x_offsets_left[4] = {0, 0, 0, 0};
    byte x_offsets_right[4] = {0, 0, 0, 0};

    for(i = 0; i < 256; i++) {
        plane_offset_clipping_left[i][0] = (byte) (plane_vals_left);
        plane_offset_clipping_left[i][1] = (byte) (plane_vals_left >> 8);
        plane_offset_clipping_left[i][2] = (byte) (plane_vals_left >> 16);
        plane_offset_clipping_left[i][3] = (byte) (plane_vals_left >> 24);
        plane_vals_left = (plane_vals_left >> 8 % 32) | (plane_vals_left << (32 - 8) % 32);  // circular rotate right 8 bits
        plane_offset_clipping_right[i][0] = (byte) (plane_vals_right);
        plane_offset_clipping_right[i][1] = (byte) (plane_vals_right >> 8);
        plane_offset_clipping_right[i][2] = (byte) (plane_vals_right >> 16);
        plane_offset_clipping_right[i][3] = (byte) (plane_vals_right >> 24);
        plane_vals_right = (plane_vals_right << 8 % 32) | (plane_vals_right >> (32 - 8) % 32);  // circular rotate left 8 bits
        x_offset_clipping_left[i][0] = x_offsets_left[0];
        x_offset_clipping_left[i][1] = x_offsets_left[1];
        x_offset_clipping_left[i][2] = x_offsets_left[2];
        x_offset_clipping_left[i][3] = x_offsets_left[3];
        x_offsets_left[3 - (i % 4)]++;
        x_offset_clipping_right[i][0] = x_offsets_right[0];
        x_offset_clipping_right[i][1] = x_offsets_right[1];
        x_offset_clipping_right[i][2] = x_offsets_right[2];
        x_offset_clipping_right[i][3] = x_offsets_right[3];
        x_offsets_right[i % 4]++;
    }
}

struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height, bool is_planar, dword compiled_size) {
    struct gfx_buffer* empty_buffer;
    dword buffer_size = compiled_size ? (dword) compiled_size : (dword) width * (dword) height;

    empty_buffer = malloc(sizeof(struct gfx_buffer) + (sizeof(byte) * buffer_size));

    empty_buffer->buffer_size = buffer_size;
    empty_buffer->buffer_flags = 0;
    empty_buffer->width = width;
    empty_buffer->height = height;

    if(is_planar) {
        empty_buffer->buffer_flags |= GFX_BUFFER_FLAG_PLANAR;
    }

    if(compiled_size > 0) {
        empty_buffer->buffer_flags |= GFX_BUFFER_FLAG_COMPILED;
    }

    /* bitmap buffer is memory immediately after main struct */
    empty_buffer->buffer = (byte *) empty_buffer + sizeof(struct gfx_buffer);

    memset(empty_buffer->buffer, 0, empty_buffer->buffer_size);

    return empty_buffer;
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

struct gfx_screen_state* _init_screen_state(byte horz_tiles, byte vert_tiles) {
    struct gfx_screen_state* screen_state;
    word tile_count = (word) horz_tiles * (word) vert_tiles;

    /* allocate consecutive memory for struct + tile index */
    dword bytes_to_allocate = sizeof(struct gfx_screen_state)
        + sizeof(struct gfx_tile_state) * tile_count;

    screen_state = (gfx_screen_state *) malloc(bytes_to_allocate);

    screen_state->tile_count = tile_count;
    screen_state->horz_tiles = horz_tiles;
    screen_state->vert_tiles = vert_tiles;
    screen_state->horz_tile_offset = 0;
    screen_state->vert_tile_offset = 0;
    screen_state->view_scroll_x = 0;
    screen_state->view_scroll_y = 0;

    /* set pointers to contiguous memory locations */
    screen_state->tile_index = (gfx_tile_state *) ((byte *) screen_state + sizeof(struct gfx_screen_state));
    memset(screen_state->tile_index, 0, sizeof(struct gfx_tile_state) * tile_count);

    _set_tile_states(screen_state, GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_TILE, FALSE, 0, 0, horz_tiles - 1, vert_tiles - 1);

    return screen_state;
}

struct gfx_tilemap* _init_tilemap(byte horz_tiles, byte vert_tiles) {
    struct gfx_tilemap* tilemap;
    word tile_count = (word) horz_tiles * (word) vert_tiles;
    dword bytes_to_allocate = sizeof(struct gfx_tilemap)
        + sizeof(byte) * tile_count;                        // memory for tilemap buffer

    tilemap = (gfx_tilemap *) malloc(bytes_to_allocate);

    tilemap->tile_count = tile_count;
    tilemap->horz_tiles = horz_tiles;
    tilemap->vert_tiles = vert_tiles;
    tilemap->buffer_size = sizeof(byte) * tile_count;

    /* bitmap buffer is memory immediately after main struct */
    tilemap->buffer = (byte *) tilemap + sizeof(struct gfx_tilemap);
    memset(tilemap->buffer, 0, tilemap->buffer_size);

    return tilemap;
}

void _gfx_add_sprite_to_draw(gfx_screen_state *screen_state, gfx_buffer *sprite_buffer, word dest_x, word dest_y, word width, word height, int x_offset, int y_offset, bool flip_horz, byte palette_offset) {
    gfx_sprite_to_draw *cur_sprite = &sprites_to_draw[sprites_to_draw_count++];
    word max_width = MIN(PAGE_WIDTH, dest_x + width) - dest_x;
    word max_height = MIN(PAGE_HEIGHT, dest_y + height) - dest_y;

    cur_sprite->sprite_buffer = sprite_buffer;
    cur_sprite->dest_x = dest_x;
    cur_sprite->dest_y = dest_y;
    cur_sprite->width = max_width;
    cur_sprite->height = max_height;
    cur_sprite->x_offset = x_offset;
    cur_sprite->y_offset = y_offset;
    cur_sprite->flip_horz = flip_horz;
    cur_sprite->palette_offset = palette_offset;
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

void gfx_load_linear_bitmap_to_planar_bitmap(byte *source_bitmap, byte *dest_bitmap, word width, word height, bool row_major) {
    byte plane;
    word x, y;
    dword offset;

    if(row_major) {
        for(plane = 0; plane < 4; plane++){
            offset = ((width * height) >> 2) * plane;
            for(y = 0; y < height; y++) {
                for(x = plane; x < width; x += 4) {
                    dest_bitmap[offset++] = source_bitmap[y * width + x];
                }
            }
        }
    } else {
        for(plane = 0; plane < 4; plane++){
            offset = ((width * height) >> 2) * plane;
            for(x = plane; x < width; x += 4) {
                for(y = 0; y < height; y++) {
                    dest_bitmap[offset++] = source_bitmap[y * width + x];
                }
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

void gfx_draw_bitmap_to_screen(gfx_buffer *bitmap, int dest_x, int dest_y, bool flip_horz, byte palette_offset) {
    int draw_x = dest_x;
    int draw_y = dest_y;
    int x_offset = 0, y_offset = 0;
    bool can_be_clipped = bitmap->buffer_flags & GFX_BUFFER_FLAG_CLIPPING;

    // do not draw bitmaps that go completely offscreen
    if((draw_x + bitmap->width) < 0
        || draw_x > SCREEN_WIDTH
        || (draw_y + bitmap->height) < 0
        || draw_y > SCREEN_HEIGHT) {
        return;
    }

    if(draw_x < 0) {
        x_offset = (0 - draw_x) * -1;
        draw_x = 0;
    } else if(draw_x + bitmap->width > 0 + SCREEN_WIDTH) {
        x_offset = bitmap->width - (SCREEN_WIDTH - draw_x);
        draw_x = 0 + SCREEN_WIDTH - bitmap->width;
    }

    if(draw_y < 0) {
        y_offset = (0 - draw_y) * -1;
        draw_y = 0;
    } else if(draw_y + bitmap->height > 0 + SCREEN_HEIGHT) {
        y_offset = bitmap->height - (SCREEN_HEIGHT - draw_y);
        draw_y = 0 + SCREEN_HEIGHT - bitmap->height;
    }

    // clear calculated offsets if bitmap does not support clipping
    _gfx_add_sprite_to_draw(
        screen_state_current,
        bitmap,
        (word) draw_x + (view_scroll_x % TILE_WIDTH),
        (word) draw_y + (view_scroll_y % TILE_HEIGHT),
        bitmap->width,
        bitmap->height,
        can_be_clipped ? x_offset : 0,
        can_be_clipped ? y_offset : 0,
        flip_horz,
        palette_offset
    );
}

void gfx_set_tile_states_for_sprites(){
    word i;
    gfx_sprite_to_draw *cur_sprite;

    for(i = 0; i < sprites_to_draw_count; i++) {

        cur_sprite = &sprites_to_draw[i];
        _set_tile_states(
            screen_state_current,
            GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE,
            FALSE,
            cur_sprite->dest_x / TILE_WIDTH,
            cur_sprite->dest_y / TILE_HEIGHT,
            (cur_sprite->dest_x + cur_sprite->width - 1) / TILE_WIDTH,
            (cur_sprite->dest_y + cur_sprite->height - 1) / TILE_HEIGHT
        );
    }
}

void _set_tile_for_screen_state(gfx_screen_state *screen_state, byte tile, byte x, byte y, bool force) {
    word tile_offset = ((word) y * (word) render_tile_width) + (word) x;
    gfx_tile_state *tile_state = &screen_state->tile_index[tile_offset];

    /* only mark tile as dirty if the tile has changed */
    if(!(tile_state->state & GFX_TILE_STATE_TILE) || tile_state->tile != tile || force) {
        tile_state->tile = tile;
        tile_state->state |= GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_TILE;
    }
}

void gfx_set_tile(byte tile, byte x, byte y) {
    _set_tile_for_screen_state(screen_state_page_0, tile, x, y, FALSE);
    _set_tile_for_screen_state(screen_state_page_1, tile, x, y, FALSE);
}

void gfx_reload_tilemap(byte x_offset, byte y_offset) {
    int i, tilemap_x, tilemap_y;
    byte tile;
    for(i = 0; i < render_tile_width * render_tile_height; i++) {
        tilemap_x = x_offset + (i % render_tile_width);
        tilemap_y = y_offset + (i / render_tile_width);
        tile = screen_tilemap->buffer[(tilemap_y * screen_tilemap->horz_tiles) + tilemap_x];
        gfx_set_tile(tile, i % render_tile_width, i / render_tile_width);
    }
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

void gfx_blit_clipped_planar_sprite(gfx_sprite_to_draw *sprite, byte *initial_vga_offset, byte selected_plane){
    byte x, x_min, x_max, y, y_min, y_max, pixel, sprite_plane, sprite_width = sprite->width >> 2;
    dword sprite_offset;
    int x_offset = sprite->x_offset;
    byte *sprite_buffer = sprite->sprite_buffer->buffer;
    byte *current_vga_offset = initial_vga_offset;

    if(sprite->y_offset < 0) {
        y_min = abs(sprite->y_offset);
        y_max = sprite->height;
    } else {
        y_min = 0;
        y_max = sprite->height - sprite->y_offset;
        current_vga_offset += (PAGE_WIDTH >> 2) * abs(sprite->y_offset);
    }

    if(x_offset < 0) {
        sprite_plane = plane_offset_clipping_left[abs(x_offset)][selected_plane];
        // overwrite x_offset with offset calculated from clipping table
        x_offset = x_offset_clipping_left[abs(x_offset)][selected_plane];
        x_min = x_offset;
        x_max = sprite_width;
        current_vga_offset -= x_offset;
    } else {
        sprite_plane = plane_offset_clipping_right[abs(x_offset)][selected_plane];
        // overwrite x_offset with offset calculated from clipping table
        x_offset = x_offset_clipping_right[abs(x_offset)][selected_plane];
        x_min = 0;
        x_max = sprite_width - x_offset;
        current_vga_offset += x_offset;
    }

    current_vga_offset += x_min;
    sprite_offset = sprite->sprite_buffer->plane_offsets[sprite_plane] + (y_min * sprite_width) + x_min;

    // for(y = y_min; y < y_max; y++) {
    //     for(x = x_min; x < x_max; x++) {
    //         pixel = sprite_buffer[sprite_offset++];
    //         if(pixel) {
    //             current_vga_offset[0] = pixel;
    //         }
    //         current_vga_offset++;
    //     }
    //     sprite_offset += sprite_width - (x_max - x_min);
    //     current_vga_offset += (PAGE_WIDTH >> 2) - (x_max - x_min);
    // }
    if(sprite->sprite_buffer->buffer_flags & GFX_BUFFER_FLAG_PALETTE_OFFSET) {
        gfx_blit_clipped_sprite_with_palette_offset(
            current_vga_offset,
            &sprite_buffer[sprite_offset],
            sprite_width,
            x_min,
            x_max,
            y_min,
            y_max,
            sprite->palette_offset
        );
    } else {
        gfx_blit_clipped_sprite(
            current_vga_offset,
            &sprite_buffer[sprite_offset],
            sprite_width,
            x_min,
            x_max,
            y_min,
            y_max
        );
    }
}

/**
 * Video initialization
 **/
void gfx_init() {
    word i;

    vga_init_modex();
    vga_wait_for_retrace();
    vga_scroll_offset(0, 0);
    render_page_width = PAGE_WIDTH;
    render_page_height = PAGE_HEIGHT;
    // gfx_screen_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, render_page_width, render_page_height, TRUE);

    /* tile atlas consists of 16x16 tiles, totalling 256 unique tiles */
    gfx_tileset_buffer = gfx_create_empty_buffer(GFX_BUFFER_BPP_8, TILE_WIDTH * 16, TILE_HEIGHT * 16, FALSE, 0);

    /* render tile area should encompass the size of the viewport + 1 tile*/
    render_tile_width = render_page_width / TILE_WIDTH;
    /* page height is 1 tile bigger than render tile height to give room
       for vertical scrolling */
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

    _init_clipping_tables();
}

void gfx_shutdown() {
    // TODO: free all memory
    vga_exit_modex();
}

/* this loads the tileset into the VRAM after the two pages */
void gfx_load_tileset() {
    byte plane;
    word i, x, y, x_current = 0, y_current = 0;
    byte *tileset_buffer = gfx_tileset_buffer->buffer;
    dword tileset_width = gfx_tileset_buffer->width;
    dword cur_offset;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);
        cur_offset = (PAGE_WIDTH >> 2) * PAGE_HEIGHT * 2;
        for(i = 0; i < 256; i++) {
            x_current = ((i % 16) * TILE_WIDTH) + plane;
            y_current = (i / 16) * TILE_HEIGHT;
            for(y = y_current; y < y_current + TILE_HEIGHT; y++) {
                for(x = x_current; x < x_current + TILE_WIDTH; x += 4) {
                    VGA[cur_offset++] = tileset_buffer[(y * tileset_width ) + x];
                }
            }
        }
    }
}

void _gfx_blit_planar_screen() {
    int plane, offset = 0;
    int initial_offset = screen_state_current->current_render_page_offset;
    int buffer_size = gfx_screen_buffer->buffer_size >> 2;
    byte *screen_buffer = gfx_screen_buffer->buffer;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);
        /* TODO: figure out how to blit one word/dword at a time! */
        memcpy(&VGA[initial_offset], &screen_buffer[offset], buffer_size);
        offset += buffer_size;
    }
}

void _gfx_blit_dirty_tiles() {
    gfx_tile_state *tile_index = screen_state_current->tile_index;
    byte tile_number;
    dword x, y, i, vga_offset, tile_offset, initial_offset = screen_state_current->current_render_page_offset;
    dword initial_tile_offset = (PAGE_WIDTH >> 2) * PAGE_HEIGHT * 2;

    /* pixel variable made volatile to prevent the compiler from
       optimizing it out, since value is not actually used */
    volatile byte pixel;

    outpw(SC_INDEX, ((word)0xff << 8) + MAP_MASK);      //select all planes
    outpw(GC_INDEX, 0x08);                              //set to OR mode

    /* TODO: this could be optimized better to use less variables */
    for(i = 0; i < render_tile_count; i++) {
        // skip tile if there is nothing to update
        if(!(tile_index[i].state & (GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2)))
            continue;
        tile_number = tile_index[i].tile;
        x = (word) (i % render_tile_width) * TILE_WIDTH;
        y = (word) (i / render_tile_width) * TILE_HEIGHT;
        vga_offset = ((y * PAGE_WIDTH + x) >> 2) + initial_offset;
        tile_offset = initial_tile_offset + ((TILE_HEIGHT) * (TILE_WIDTH >> 2) * tile_number);
        // for(y = 0; y < TILE_HEIGHT; y++) {
        //     for(x = 0; x < TILE_WIDTH >> 2; x++) {
        //         pixel = VGA[tile_offset++];             //read pixel to load all the latches
        //         VGA[vga_offset + x] = 0;                //write four pixels in parallel
        //     }
        //     vga_offset += PAGE_WIDTH >> 2;
        // }
        gfx_blit_16_x_16_tile(&VGA[vga_offset], &VGA[tile_offset]);

        // clear tile state once it has been blitted
        tile_index[i].state &= ~(GFX_TILE_STATE_DIRTY_1 | GFX_TILE_STATE_DIRTY_2 | GFX_TILE_STATE_SPRITE);
    }

    outpw(GC_INDEX + 1, 0x0ff);
}

void _blit_sprite_onto_plane(gfx_sprite_to_draw *sprite, byte plane) {
    byte *sprite_buffer = sprite->sprite_buffer->buffer;
    dword initial_vga_offset, sprite_offset;
    dword selected_plane = plane - (sprite->dest_x % 4);
    /* modulus again to account for overflow, can't do it on same line as above for some reason,
        it's likely getting optimized out (at least on Watcom) */
    selected_plane = selected_plane % 4;

    initial_vga_offset = screen_state_current->current_render_page_offset
        + (sprite->dest_y * (PAGE_WIDTH >> 2))
        + ((sprite->dest_x + selected_plane) >> 2);

    // if an offset has been set, then the sprite is to be clipped
    if(sprite->sprite_buffer->buffer_flags & GFX_BUFFER_FLAG_CLIPPING && (sprite->x_offset != 0 || sprite->y_offset != 0)) {
        gfx_blit_clipped_planar_sprite(sprite, &VGA[initial_vga_offset], selected_plane);
    } else {
        sprite_offset = sprite->sprite_buffer->plane_offsets[selected_plane];

        if(sprite->sprite_buffer->buffer_flags & GFX_BUFFER_FLAG_COMPILED) {
            gfx_blit_compiled_planar_sprite_scheme_2(
                &VGA[initial_vga_offset],
                &sprite_buffer[sprite_offset + ((dword) (sprite->width >> 2) * (dword) sprite->height)],
                &sprite_buffer[sprite_offset],
                sprite->palette_offset
            );
        } else {
            gfx_blit_sprite(
                &VGA[initial_vga_offset],
                &sprite_buffer[sprite_offset],
                (byte) sprite->height,
                (byte) (sprite->width >> 2)
            );
        }
    }
}

void gfx_blit_sprites() {
    byte plane = 0;
    word i;

    for(plane = 0; plane < 4; plane++) {
        // select one plane at a time
        outp(SC_INDEX, MAP_MASK);
        outp(SC_DATA, 1 << plane);

        for(i = 0; i < sprites_to_draw_count; i++) {
            _blit_sprite_onto_plane(&sprites_to_draw[i], plane);
        }
    }
}

// TODO: handle case where tile offsets go beyond the render tile width or height
void _scroll_screen_tiles(gfx_screen_state* screen_state, gfx_tilemap* tilemap, int horz_tile_offset, int vert_tile_offset) {
    byte x, y, i;
    byte abs_horz_tile_offset = (byte) abs(horz_tile_offset);
    word tile_index_offset = 0;
    word tilemap_offset;
    word tile_row_size = (PAGE_WIDTH >> 2) * TILE_HEIGHT;
    byte *tilemap_buffer = tilemap->buffer;
    int new_render_page_offset;
    bool bottom_of_page = FALSE;
    bool top_of_page = FALSE;

    // new_render_page_offset = (int) screen_state->current_render_page_offset + (horz_tile_offset * TILE_WIDTH >> 2) + (vert_tile_offset * (((render_tile_width * TILE_WIDTH) >> 2) * TILE_HEIGHT));
    new_render_page_offset = (int) screen_state->current_render_page_offset + (horz_tile_offset * TILE_WIDTH >> 2);

    /* bounds checks to determine whether or not new offset will
       push rendering past bottom or top of its page*/
    bottom_of_page = new_render_page_offset - (int) screen_state->initial_render_page_offset > (int) tile_row_size;
    top_of_page = new_render_page_offset < (int) screen_state->initial_render_page_offset;

    if(horz_tile_offset != 0) {
        tilemap_offset = screen_state->vert_tile_offset * tilemap->vert_tiles + screen_state->horz_tile_offset + (horz_tile_offset < 0 ? -1 * (abs_horz_tile_offset) : render_tile_width);
        for(y = 0; y < render_tile_height; y++) {        
            if(horz_tile_offset < 0) {
                /* TODO: handle case where offset goes beyond the limit */
                /* shift all tiles to the right */
                for(x = render_tile_width - 1; x > (abs_horz_tile_offset - 1); x--) {
                    screen_state->tile_index[tile_index_offset + x] = screen_state->tile_index[tile_index_offset + x - abs_horz_tile_offset];
                }
                /* populate incoming tiles on the left */
                for(i = 0; i < abs_horz_tile_offset; i++)
                    _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + i], i, y, TRUE);
            } else {
                /* TODO: handle case where offset goes below zero and wraps around */
                /* shift all tiles to the left */
                for(x = abs_horz_tile_offset; x < render_tile_width; x++) {
                    screen_state->tile_index[tile_index_offset + x - abs_horz_tile_offset] = screen_state->tile_index[tile_index_offset + x];
                }
                /* populate incoming tiles on the right */
                for(i = 0; i < abs_horz_tile_offset; i++)
                    _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + i], render_tile_width - (abs_horz_tile_offset - i), y, TRUE);
            }
            tile_index_offset += render_tile_width;
            tilemap_offset += tilemap->horz_tiles;
        }
    }

    if(bottom_of_page || top_of_page || vert_tile_offset != 0) {
        tilemap_offset = (screen_state->vert_tile_offset + vert_tile_offset) * tilemap->vert_tiles + screen_state->horz_tile_offset + horz_tile_offset;
        for(y = 0; y < render_tile_height; y++) {
            for(x = 0; x < render_tile_width; x++) {
                _set_tile_for_screen_state(screen_state, tilemap_buffer[tilemap_offset + x], x, y, FALSE);
            }
            tilemap_offset += tilemap->horz_tiles;
        }

        if(bottom_of_page) {
            new_render_page_offset -= tile_row_size;
        } else if (top_of_page) {
            new_render_page_offset += tile_row_size;
        }
    }

    /* set horizontal and vertical offset for screen state */
    screen_state->horz_tile_offset += horz_tile_offset;
    screen_state->vert_tile_offset += vert_tile_offset;
    screen_state->current_render_page_offset = new_render_page_offset;
}

void gfx_set_scroll_offset(word x_offset, word y_offset) {
    int horz_tile_offset = (x_offset / TILE_WIDTH) - (screen_state_current->view_scroll_x / TILE_WIDTH);
    int vert_tile_offset = (y_offset / TILE_HEIGHT) - (screen_state_current->view_scroll_y / TILE_HEIGHT);

    if(horz_tile_offset != 0 || vert_tile_offset != 0) {
        _scroll_screen_tiles(screen_state_current, screen_tilemap, horz_tile_offset, vert_tile_offset);
    }

    view_scroll_x = x_offset;
    view_scroll_y = y_offset;
    screen_state_current->view_scroll_x = view_scroll_x;
    screen_state_current->view_scroll_y = view_scroll_y;
}

/**
 * Main rendering call
 **/
void gfx_render_all() {
    word vga_offset_x = ((screen_state_current->view_scroll_x % TILE_WIDTH) >> 2);
    word vga_offset_y = ((screen_state_current->view_scroll_y % TILE_HEIGHT) * TILE_WIDTH * render_tile_width) >> 2;

    vga_wait_for_retrace();

    /* blitting */
    _gfx_blit_dirty_tiles();
    if(sprites_to_draw_count > 0) {
        gfx_blit_sprites();
        gfx_set_tile_states_for_sprites();
        /* clear buffers once rendering has finished */
        sprites_to_draw_count = 0;
    }

    /* page flip + scrolling */
    vga_set_offset(screen_state_current->current_render_page_offset + vga_offset_x + vga_offset_y);
    vga_set_horizontal_pan((byte) screen_state_current->view_scroll_x);

    current_render_page = 1 - current_render_page;
    screen_state_current = current_render_page ? screen_state_page_1 : screen_state_page_0;
    frame_number++;
}
