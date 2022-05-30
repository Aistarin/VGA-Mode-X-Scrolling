#ifndef GFX_H_
#define GFX_H_

#include "common.h"

#define TILE_WIDTH          16              // global tile width (in pixels)
#define TILE_HEIGHT         16              // global tile height (in pixels)

enum gfx_color_depths {
    GFX_BUFFER_BPP_8,
    GFX_BUFFER_BPP_15,
    GFX_BUFFER_BPP_16,
    GFX_BUFFER_BPP_24,
    GFX_BUFFER_BPP_32
};

enum gfx_tile_states {
    GFX_TILE_STATE_TILE     = 0x01,         // screen tile has a tile assigned to it
    GFX_TILE_STATE_SPRITE   = 0x02,         // screen tile has a sprite or bitmap drawn to it
    GFX_TILE_STATE_DIRTY_1  = 0x04,         // screen tile has been modified with a tile
    GFX_TILE_STATE_DIRTY_2  = 0x08          // screen tile has been modified with a sprite or bitmap
};

int gfx_color_depth_sizes[] = {1, 2, 2, 3, 4};

typedef struct gfx_buffer {
    int color_depth;                        // color depth of image
    dword buffer_size;                      // total size of buffer (in bytes)
    bool is_planar;                         // flag to determine whether or not buffer is stored in planar format
    word width;                             // buffer width (in pixels)
    word height;                            // buffer height (in pixels)
    byte *buffer;                           // array of bytes that holds raw bitmap data
} gfx_buffer;

typedef struct gfx_sprite_to_draw {
    gfx_buffer *sprite_buffer;              // pointer to planar sprite bitmap data
    word dest_x;                            // x destination coords of sprite
    word dest_y;                            // y destination coords of sprite
    word width;
    word height;
} gfx_sprite_to_draw;

typedef struct gfx_tile_state {
    word tile;                              // tileset index
    byte color;                             // default color
    byte state;                             // state of tile
} gfx_tile_state;

typedef struct gfx_screen_state {
    dword render_page_offset;               // current VRAM offset
    word tile_count;                        // number of tiles total
    byte horz_tiles;                        // number of horizontal tiles
    byte vert_tiles;                        // number of vertical tiles
    word tiles_to_clear_count;              // number of tiles to clear before drawing sprites
    word tiles_to_update_count;             // number of tiles to update
    word sprites_to_draw_count;             // number of sprites to draw
    gfx_tile_state *tile_index;             // main tile index
    word *tiles_to_clear;                   // list of tiles to clear
    word *tiles_to_update;                  // list of tiles to update
    gfx_sprite_to_draw *sprites_to_draw;    // buffer of sprites to draw on screen
} gfx_screen_state;

void gfx_init_video();
struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height, bool is_planar);
void gfx_blit_buffer_to_active_page(gfx_buffer* buffer, word dest_x, word dest_y);
gfx_buffer* gfx_get_screen_buffer();
gfx_buffer* gfx_get_tileset_buffer();
void gfx_set_tile(byte tile, byte x, byte y);
void gfx_blit_screen_buffer();
void gfx_mirror_page();
void gfx_render_all();
void gfx_load_tileset();
void gfx_draw_sprite_to_screen(gfx_buffer *bitmap, word source_x, word source_y, word dest_x, word dest_y, word width, word height);
void gfx_draw_planar_sprite_to_planar_screen(gfx_buffer *sprite_bitmap, word x, word y);

#endif
