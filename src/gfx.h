#ifndef GFX_H_
#define GFX_H_

#include "common.h"

typedef struct gfx_buffer_8bit {
    dword buffer_size;                      // total size of buffer in bytes
    byte is_planar;                         // flag to determine whether or not buffer is stored in planar format
    word width;
    word height;
    byte buffer[];
} gfx_buffer_8bit;

struct gfx_buffer_8bit* gfx_create_empty_buffer_8bit(word width, word height);

void gfx_blit_buffer_to_active_page(gfx_buffer_8bit* buffer, word dest_x, word dest_y);

#endif
