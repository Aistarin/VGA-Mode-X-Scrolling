#ifndef SPR_H_
#define SPR_H_

#include "src/common.h"

dword spr_compile_planar_sprite_scheme_1(byte *sprite_buffer, word width, word height, byte *output_buffer, dword *plane_offsets);
dword spr_compile_planar_sprite_scheme_2(byte *sprite_buffer, word width, word height, byte *output_buffer, dword *plane_offsets, byte palette_offset, bool palette_swappable);

#endif
