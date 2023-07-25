#include "src/common.h"
#include "src/ecs/ecs.h"
#include "src/gfx/gfx.h"
#include "src/gfx/vga.h"
#include "src/io/bitmap.h"
#include "src/io/file.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

bool exit_program = FALSE;
byte palette[256*3];

int view_pos_x = 0;
int view_pos_y = 0;

gfx_buffer *tileset_buffer;
gfx_tilemap *tilemap_buffer;

void handle_input() {
    if(kbhit()) {
        switch(getch()) {
            case 27:    // esc
                exit_program = TRUE;
                break;
        }
    }
}

void handle_logic() {
    // TODO
}

void handle_graphics() {
    int i;
    // entity *entity_current;

    /* scroll screen before drawing sprites */
    gfx_set_scroll_offset(view_pos_x, view_pos_y);

    // for(i = 0; i < entity_count; i++) {
    //     entity_current = &entity_list[i];
    //     gfx_draw_sprite_to_screen(
    //         entity_current->sprite_buffer,
    //         0,
    //         0,
    //         (word) entity_current->xpos,
    //         (word) entity_current->ypos,
    //         entity_current->sprite_buffer->width,
    //         entity_current->sprite_buffer->height,
    //         entity_current->sprite_flip
    //     );
    // }

    gfx_render_all();
}

int main(int argc, char *argv[]) {
    ecs_init();
    gfx_init();

    tileset_buffer = gfx_get_tileset_buffer();
    tilemap_buffer = gfx_get_tilemap_buffer();

    load_bmp_to_buffer("testtile.bmp", tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height, palette);
    vga_set_palette(palette, 0, 255);
    gfx_load_tileset();

    read_bytes_from_file("test.map", tilemap_buffer->buffer, tilemap_buffer->buffer_size);
    gfx_reload_tilemap(0, 0);

    while(!exit_program) {
        handle_input();
        handle_logic();
        handle_graphics();
    }

    ecs_shutdown();
    gfx_shutdown();

    return 0;
}
