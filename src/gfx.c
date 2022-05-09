#include "gfx.h"
#include "vga.h"
#include <stdio.h>
#include <stdlib.h>

gfx_buffer_8bit *screen_buffer;

void gfx_init_video() {
    vga_init_modex();
    vga_scroll_offset(0, 0);
    screen_buffer = gfx_create_empty_buffer_8bit(PAGE_WIDTH, PAGE_HEIGHT);
};

struct gfx_buffer_8bit* gfx_create_empty_buffer_8bit(word width, word height) {
    int buffer_size;
    struct gfx_buffer_8bit* empty_buffer;

    buffer_size = ((int) width * (int) height) * sizeof(byte);
    empty_buffer = malloc(sizeof(gfx_buffer_8bit) + buffer_size);

    empty_buffer->buffer_size = buffer_size;
    empty_buffer->is_planar = 0;
    empty_buffer->width = width;
    empty_buffer->height = height;

    return empty_buffer;
};

void gfx_blit_buffer_to_active_page(gfx_buffer_8bit* buffer, word dest_x, word dest_y) {
    vga_blit_buffer_to_vram(buffer->buffer, buffer->width, buffer->height, 0, 0, dest_x, dest_y, buffer->width, buffer->height);  
};
