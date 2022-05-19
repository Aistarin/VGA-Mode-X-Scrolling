#ifndef GFX_H_
#define GFX_H_

#include "common.h"

#define TILE_WIDTH          16              // global tile width (in pixels)
#define TILE_HEIGHT         16              // global tile height (in pixels)

enum gfx_draw_commands {
    GFX_DRAW_COLOR,
    GFX_DRAW_TILE,
    GFX_BLIT_TILES,
    GFX_BLIT_BUFFER,
    GFX_MIRROR_PAGE
};

enum gfx_color_depths {
    GFX_BUFFER_BPP_8,
    GFX_BUFFER_BPP_15,
    GFX_BUFFER_BPP_16,
    GFX_BUFFER_BPP_24,
    GFX_BUFFER_BPP_32
};

enum gfx_tile_states {
    GFX_TILE_STATE_DIRTY_1  = 0x01,         // screen tile has been modified
    GFX_TILE_STATE_DIRTY_2  = 0x02,         // screen tile has been modified
    GFX_TILE_STATE_TILE     = 0x04,         // screen tile has a tile assigned to it
    GFX_TILE_STATE_SPRITE   = 0x08          // screen tile has had a sprite drawn to it
};

int gfx_color_depth_sizes[] = {1, 2, 2, 3, 4};

typedef struct gfx_buffer {
    int color_depth;                        // color depth of image
    dword buffer_size;                      // total size of buffer (in bytes)
    byte is_planar;                         // flag to determine whether or not buffer is stored in planar format
    word width;                             // buffer width (in pixels)
    word height;                            // buffer height (in pixels)
    byte *buffer;                           // array of bytes that holds raw bitmap data
} gfx_buffer;

typedef struct gfx_draw_command {
    byte command;
    byte arg0;
    byte arg1;
    byte arg2;
} gfx_draw_command;

void gfx_init_video();
struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height);
void gfx_blit_buffer_to_active_page(gfx_buffer* buffer, word dest_x, word dest_y);
gfx_buffer* gfx_get_screen_buffer();
gfx_buffer* gfx_get_tileset_buffer();
void gfx_set_tile(byte tile, byte x, byte y);
void gfx_blit_screen_buffer();
void gfx_mirror_page();
void gfx_render_all();
void gfx_render_all_test();
void gfx_load_tileset();
void gfx_draw_bitmap_to_screen(gfx_buffer *bitmap, word source_x, word source_y, word dest_x, word dest_y, word width, word height);

#endif
