#ifndef SPR_H_
#define SPR_H_

#include "common.h"

dword spr_compile_planar_sprite(byte *sprite_buffer, word width, word height, byte *output_buffer, dword *plane_offsets);

#endif
