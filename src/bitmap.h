#ifndef BITMAP_H_
#define BITMAP_H_

#include "src/common.h"

void load_bmp_to_buffer(char *file, byte *screen_buffer, word buffer_width, word buffer_height, byte *palette);

#endif
