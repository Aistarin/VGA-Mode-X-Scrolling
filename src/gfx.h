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

typedef struct gfx_buffer_8bit {
    dword buffer_size;                      // total size of buffer in bytes
    byte is_planar;                         // flag to determine whether or not buffer is stored in planar format
    word width;                             // buffer width (in pixels)
    word height;                            // buffer height (in pixels)
    byte buffer[1];                         // array of bytes that holds raw bitmap data
} gfx_buffer_8bit;

typedef struct gfx_tile_screen {
    byte tile_width;                        // tile width (in pixels)
    byte tile_height;                       // tile height (in pixels)
    byte tile_count_horz;                   // number of horizontal tiles
    byte tile_count_vert;                   // number of vertical tiles
    byte tilemap[1];                        // tile indexes currently on screen
    byte tilestate[1];                      // bitmap of current tile states
} gfx_tile_screen;

typedef struct gfx_draw_command {
    byte command;
    byte arg0;
    byte arg1;
    byte arg2;
} gfx_draw_command;

void gfx_init_video();
struct gfx_buffer_8bit* gfx_create_empty_buffer_8bit(word width, word height);
void gfx_blit_buffer_to_active_page(gfx_buffer_8bit* buffer, word dest_x, word dest_y);
byte* gfx_get_screen_buffer();
byte* gfx_get_tileset_buffer();
void gfx_blit_screen_buffer();
void gfx_mirror_page();
void gfx_render_all();

#endif
