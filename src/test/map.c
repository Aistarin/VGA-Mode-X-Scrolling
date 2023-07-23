#include "src/common.h"
#include "src/gfx/gfx.h"
#include "src/gfx/vga.h"
#include "src/io/bitmap.h"
#include <conio.h>
#include <stdio.h>

int view_pos_x = 0;
int view_pos_y = 0;
int tile_cursor_x = 0;
int tile_cursor_y = 0;
int tile_cursor_x_rel = 0;
int tile_cursor_y_rel = 0;
int tile_cursor_x_rel_max = (PAGE_WIDTH / TILE_WIDTH) - 2;
int tile_cursor_y_rel_max = ((PAGE_HEIGHT / TILE_HEIGHT) - 3);
byte tile_cursor_index = 1;
byte tile_tilemap_index = 0;
gfx_buffer *tileset_buffer;
gfx_tilemap *tilemap_buffer;

byte get_tile_at_pos(int x, int y) {
    return tilemap_buffer->buffer[(y * (int) tilemap_buffer->horz_tiles) + x];
}

void write_tile() {
    gfx_set_tile(tile_cursor_index, (byte) tile_cursor_x_rel, (byte) tile_cursor_y_rel);
    tilemap_buffer->buffer[(tile_cursor_y * (int) tilemap_buffer->horz_tiles) + tile_cursor_x] = tile_cursor_index;
    tile_tilemap_index = tile_cursor_index;
}

void move_tile_cursor(int x_delta, int y_delta) {
    gfx_set_tile(tile_tilemap_index, (byte) tile_cursor_x_rel, (byte) tile_cursor_y_rel);

    tile_cursor_x += x_delta;
    tile_cursor_y += y_delta;
    tile_cursor_x_rel += x_delta;
    tile_cursor_y_rel += y_delta;

    if(tile_cursor_x_rel > tile_cursor_x_rel_max) {
        tile_cursor_x_rel = tile_cursor_x_rel_max;
        view_pos_x += TILE_WIDTH;
    }
    if(tile_cursor_y_rel > tile_cursor_y_rel_max) {
        tile_cursor_y_rel = tile_cursor_y_rel_max;
        view_pos_y += TILE_HEIGHT;
    }
    if(tile_cursor_x_rel < 0) {
        tile_cursor_x_rel = 0;
        view_pos_x -= TILE_WIDTH;
    }
    if(tile_cursor_y_rel < 0) {
        tile_cursor_y_rel = 0;
        view_pos_y -= TILE_HEIGHT;
    }

    if (tile_cursor_x < 0) {
        tile_cursor_x = 0;
    }
    if (tile_cursor_y < 0) {
        tile_cursor_y = 0;
    }

    if (view_pos_x < 0) {
        view_pos_x = 0;
    }

    if(view_pos_y < 0) {
        view_pos_y = 0;
    }

    tile_tilemap_index = get_tile_at_pos(tile_cursor_x, tile_cursor_y);
}

int main(int argc, char *argv[]) {
    byte palette[256*3];
    bool exit_program = FALSE;
    bool tile_blink = FALSE;
    int tile_blink_timer = 0;

    gfx_init();

    tileset_buffer = gfx_get_tileset_buffer();
    tilemap_buffer = gfx_get_tilemap_buffer();
    load_bmp_to_buffer("testtile.bmp", tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height, palette);
    vga_set_palette(palette, 0, 255);
    gfx_load_tileset();

    while(!exit_program) {
        if(tile_blink_timer++ % 15 == 0) {
            tile_blink ^= 1;
        }

        if(tile_blink) {
            gfx_set_tile(tile_cursor_index, (byte) tile_cursor_x_rel, (byte) tile_cursor_y_rel);
        } else {
            gfx_set_tile(tile_tilemap_index, (byte) tile_cursor_x_rel, (byte) tile_cursor_y_rel);
        }

        if(kbhit()) {
            switch(getch()) {
                case 27:    // esc
                    exit_program = TRUE;
                    break;
                case 119:   // w
                    move_tile_cursor(0, -1);
                    break;
                case 115:   // s
                    move_tile_cursor(0, 1);
                    break;
                case 97:    // a
                    move_tile_cursor(-1, 0);
                    break;
                case 100:   // d
                    move_tile_cursor(1, 0);
                    break;
                case 113:   // q
                    tile_cursor_index--;
                    break;
                case 101:   // e
                    tile_cursor_index++;
                    break;
                case 32:    // space
                    write_tile();
                    break;
                case 87:    // W
                    write_tile();
                    move_tile_cursor(0, -1);
                    break;
                case 83:    // S
                    write_tile();
                    move_tile_cursor(0, 1);
                    break;
                case 65:    // A
                    write_tile();
                    move_tile_cursor(-1, 0);
                    break;
                case 68:    // D
                    write_tile();
                    move_tile_cursor(1, 0);
                    break;
            }
        }

        gfx_set_scroll_offset(view_pos_x, view_pos_y);
        gfx_render_all();
    }

    gfx_shutdown();

    return 0;
}
