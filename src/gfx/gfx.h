#ifndef GFX_H_
#define GFX_H_

#include "src/common.h"

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
    GFX_TILE_STATE_TILE             = 0x01, // screen tile has a tile assigned to it
    GFX_TILE_STATE_SPRITE           = 0x02, // screen tile has a sprite or bitmap drawn to it
    GFX_TILE_STATE_DIRTY_1          = 0x04, // screen tile has been modified with a tile
    GFX_TILE_STATE_DIRTY_2          = 0x08  // screen tile has been modified with a sprite or bitmap
};

enum gfx_sprite_commands {
    GFX_SPRITE_CMD_READ             = 0x00, // read MSB nibble as single pixel data
    GFX_SPRITE_CMD_RSEQ             = 0x01, // read following bytes sequentially as 2 pixels, next byte holding length
    GFX_SPRITE_CMD_REP              = 0x02, // repeat pixel in MSB nibble, next byte holding length
    GFX_SPRITE_CMD_SKIP             = 0x03, // skip ahead a number of pixels, reading MSB nibble as length (max 16)
    GFX_SPRITE_CMD_SSEQ             = 0x04, // skip ahead a number of pixels, next byte holding length
    GFX_SPRITE_CMD_END              = 0x05  // column has come to an end
};

enum gfx_buffer_flags {
    GFX_BUFFER_FLAG_PLANAR          = 0x01, // buffer is in planar format
    GFX_BUFFER_FLAG_COMPILED        = 0x02, // buffer contains compiled instructions
    GFX_BUFFER_FLAG_CLIPPING        = 0x04, // buffer supports clipping/has a clipping fallback
    GFX_BUFFER_FLAG_ROW_MAJOR       = 0x08  // planar data is stored in row-major order
};

typedef struct gfx_buffer {
    int color_depth;                        // color depth of image
    dword buffer_size;                      // total size of buffer (in bytes)
    byte buffer_flags;                      // bit flags that describe the buffer data
    word width;                             // buffer width (in pixels)
    word height;                            // buffer height (in pixels)
    dword plane_offsets[4];                 // plane offsets
    byte *buffer;                           // array of bytes that holds raw bitmap data and/or compiled instructions
} gfx_buffer;

typedef struct gfx_planar_sprite {
    dword buffer_size;                      // total size of buffer (in bytes)
    word width;                             // sprite width (in pixels)
    word height;                            // sprite height (in pixels)
    byte *buffer;                           // array of bytes that holds the binary sprite data interweaved with commands
} gfx_planar_sprite;

typedef struct gfx_sprite_to_draw {
    gfx_buffer *sprite_buffer;              // pointer to planar sprite bitmap data
    word dest_x;                            // x destination coords of sprite
    word dest_y;                            // y destination coords of sprite
    word width;
    word height;
    int x_offset;
    int y_offset;
    bool flip_horz;
} gfx_sprite_to_draw;

typedef struct gfx_tile_state {
    word tile;                              // tileset index
    byte color;                             // default color
    byte state;                             // state of tile
} gfx_tile_state;

typedef struct gfx_screen_state {
    dword initial_render_page_offset;       // initial VRAM offset
    dword current_render_page_offset;       // current VRAM offset
    word tile_count;                        // number of tiles total
    byte horz_tiles;                        // number of horizontal tiles
    byte vert_tiles;                        // number of vertical tiles
    gfx_tile_state *tile_index;             // main tile index
} gfx_screen_state;

typedef struct gfx_tilemap {
    word tile_count;                        // number of tiles total
    byte horz_tiles;                        // number of horizontal tiles
    byte vert_tiles;                        // number of vertical tiles
    byte horz_offset;                       // current horizontal tile offset
    byte vert_offset;                       // current vertical tile offset
    dword buffer_size;                      // total size of buffer (in bytes)
    byte *buffer;                           // array of bytes that holds the tilemap data
} gfx_tilemap;

void gfx_init(void);
void gfx_shutdown(void);
struct gfx_buffer* gfx_create_empty_buffer(int color_depth, word width, word height, bool is_planar, dword compiled_size);
void gfx_blit_buffer_to_active_page(gfx_buffer* buffer, word dest_x, word dest_y);
gfx_buffer* gfx_get_screen_buffer(void);
gfx_buffer* gfx_get_tileset_buffer(void);
void gfx_set_tile(byte tile, byte x, byte y);
void gfx_blit_screen_buffer(void);
void gfx_mirror_page(void);
void gfx_render_all(void);
void gfx_load_tileset(void);
void gfx_reload_tilemap(byte x_offset, byte y_offset);
void gfx_draw_sprite_to_screen(gfx_buffer *bitmap, word source_x, word source_y, word dest_x, word dest_y, word width, word height, int x_offset, int y_offset, bool flip_horz);
void gfx_draw_planar_sprite_to_planar_screen(gfx_buffer *sprite_bitmap, word x, word y);
void gfx_load_linear_bitmap_to_planar_bitmap(byte *source_bitmap, byte *dest_bitmap, word width, word height, bool row_major);
void gfx_set_scroll_offset(word x_offset, word y_offset);
gfx_tilemap* gfx_get_tilemap_buffer(void);

extern void gfx_blit_sprite(byte *initial_vga_offset, byte *sprite_offset, byte sprite_width, byte sprite_height);
extern void gfx_blit_16_x_16_tile(byte *vga_offset, byte *tile_offset);
extern void gfx_blit_8_x_8_tile(byte *vga_offset, byte *tile_offset);
extern void gfx_blit_compiled_planar_sprite_scheme_1(byte *vga_offset, byte *sprite_offset, dword iter);
extern void gfx_blit_compiled_planar_sprite_scheme_2(byte *vga_offset, byte *sprite_offset, byte *sprite_data_offset);

#endif
