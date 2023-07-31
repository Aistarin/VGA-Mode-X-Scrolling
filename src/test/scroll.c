#include "src/common.h"
#include "src/gfx/vga.h"
#include "src/gfx/gfx.h"
#include "src/gfx/spr.h"
#include "src/io/timer.h"
#include "src/io/keyboard.h"
#include "src/io/bitmap.h"
#include "src/test/pattern.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct testobj {
    int hspeed;
    int vspeed;
    int xpos;
    int ypos;
} testobj;

int shutdown_handler(void) {
    timer_shutdown();
    keyboard_shutdown();
    gfx_shutdown();

    return 0;
}

int init_handler(void) {
    timer_init();
    keyboard_init();
    gfx_init();

    return 0;
}

int test_scroll(int testobj_max, byte test_mode){
    int i, j, k, offset, x, y, tile_offset_x = 0, tile_offset_y = 0, pos_x = 0, pos_y = 0;
    gfx_buffer *tileset_buffer;
    gfx_buffer *sprite_buffer;
    byte *scratch_buffer;
    gfx_tilemap *tilemap_buffer;
    byte palette[256*3];
    char ch;
    int testobj_count = 0;
    dword compiled_sized = 0;
    bool exit_program = FALSE;
    word sprite_width=32, sprite_height=56;
    word render_tile_width = PAGE_WIDTH / TILE_WIDTH;
    word render_tile_height = PAGE_HEIGHT / TILE_HEIGHT;
    int speed_multiplier = 0;

    testobj *testobj_list = malloc(sizeof(testobj) * testobj_max);
    testobj *cur_testobj;

    init_handler();

    scratch_buffer = malloc(0xFFFF);

    load_bmp_to_buffer("jodi-spr.bmp", scratch_buffer, sprite_width, sprite_height, palette);
    compiled_sized = spr_compile_planar_sprite_scheme_2(scratch_buffer, sprite_width, sprite_height, NULL, NULL);

    sprite_buffer = gfx_create_empty_buffer(0, sprite_width, sprite_height, TRUE, compiled_sized);
    spr_compile_planar_sprite_scheme_2(scratch_buffer, sprite_buffer->width, sprite_buffer->height, sprite_buffer->buffer, sprite_buffer->plane_offsets);

    // gfx_load_linear_bitmap_to_planar_bitmap(scratch_buffer, sprite_buffer->buffer, sprite_buffer->width, sprite_buffer->height);

    palette[0] = 0;
    palette[1] = 0;
    palette[2] = 0;
    vga_set_palette(palette, 0, 15);

    tileset_buffer = gfx_get_tileset_buffer();

    if(test_mode == 3) {
        load_bmp_to_buffer("jodi.bmp", scratch_buffer, tileset_buffer->width, tileset_buffer->height, palette);
        vga_set_palette(palette, 0, 255);
    } else if (test_mode == 2) {
        render_pattern_to_buffer_3(scratch_buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    } else if (test_mode == 1) {
        render_pattern_to_buffer_2(scratch_buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    } else if (test_mode == 0) {
        render_pattern_to_buffer_1(scratch_buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    }

    memcpy(tileset_buffer->buffer, scratch_buffer, tileset_buffer->buffer_size);

    gfx_load_tileset();

    tilemap_buffer = gfx_get_tilemap_buffer();

    for(i = 0; i < 256; i++){
        x = i % 16;
        y = i / 16;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x] = (byte) i;
        tilemap_buffer->buffer[(y + 16) * tilemap_buffer->horz_tiles + x] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 16] = (byte) i;
        tilemap_buffer->buffer[(y + 16) * tilemap_buffer->horz_tiles + x + 16] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 32] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 48] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 64] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 80] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 96] = (byte) i;
        tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x + 112] = (byte) i;
    }

    for(i = 0; i < (render_tile_width * (render_tile_height - 1)); i++) {
        x = i % render_tile_width;
        y = i / render_tile_width;
        gfx_set_tile(tilemap_buffer->buffer[y * tilemap_buffer->horz_tiles + x], x, y);
    }

    pos_x = 0;
    pos_y = 0;
    tile_offset_x = 0;
    tile_offset_y = 0;
    i = 0;
    timer_set_interval(16);
    while (1) {
        timer_start();

        while(timer_step()) {

            speed_multiplier = !is_pressing_lshift();

            if(is_pressing_w()) {
                pos_y -= speed_multiplier ? 1 : TILE_HEIGHT;
            }
            if(is_pressing_s()) {
                pos_y += speed_multiplier ? 1 : TILE_HEIGHT;
            }
            if(is_pressing_a()) {
                pos_x -= speed_multiplier ? 1 : TILE_WIDTH;
            }
            if(is_pressing_d()) {
                pos_x += speed_multiplier ? 1 : TILE_WIDTH;
            }
            if(is_pressing_escape()) {
                exit_program = TRUE;
            }

            if(pos_y < 0)
                pos_y = 0;
            if(pos_x < 0)
                pos_x = 0;

            for(j = 0; j < testobj_count; j++) {
                cur_testobj = &testobj_list[j];
                cur_testobj->xpos += cur_testobj->hspeed;
                cur_testobj->ypos += cur_testobj->vspeed;
                if(cur_testobj->xpos > (SCREEN_WIDTH - sprite_buffer->width) || cur_testobj->xpos < 0) {
                    if(cur_testobj->xpos > (SCREEN_WIDTH - sprite_buffer->width)) cur_testobj->xpos = (SCREEN_WIDTH - sprite_buffer->width);
                    else if(cur_testobj->xpos < 0) cur_testobj->xpos = 0;
                    cur_testobj->hspeed = -(cur_testobj->hspeed);
                }

                if(cur_testobj->ypos > (SCREEN_HEIGHT - sprite_buffer->height) || cur_testobj->ypos < 0) {
                    if(cur_testobj->ypos > (SCREEN_HEIGHT - sprite_buffer->height)) cur_testobj->ypos = (SCREEN_HEIGHT - sprite_buffer->height);
                    else if(cur_testobj->ypos < 0) cur_testobj->ypos = 0;
                    cur_testobj->vspeed = -(cur_testobj->vspeed);
                }
            }
            if(k++ % 30 == 0 && testobj_count < testobj_max){
                cur_testobj = &testobj_list[testobj_count++];
                cur_testobj->xpos = rand() % (SCREEN_WIDTH - sprite_buffer->width);
                cur_testobj->ypos = rand() % (SCREEN_HEIGHT - sprite_buffer->height);
                cur_testobj->hspeed = 1 + rand() % 5;
                cur_testobj->vspeed = 1 + rand() % 5;
            }
        }

        /* scroll screen before drawing sprites */
        gfx_set_scroll_offset(pos_x, pos_y);
        for(j = 0; j < testobj_count; j++) {
            cur_testobj = &testobj_list[j];
            gfx_draw_bitmap_to_screen(sprite_buffer, (word) cur_testobj->xpos, (word) cur_testobj->ypos, FALSE);
        }
        gfx_render_all();

        timer_end();

        if(exit_program) {
            break;
        }
    }

    shutdown_handler();

    printf("total objects rendered: %d\n", testobj_count);

    return 0;
}

int main(int argc, char *argv[]) {
    char *a = argv[1];
    char *b = argv[2];
    int num = atoi(a);
    int test_mode = atoi(b);

    if(argc >= 2)
        return test_scroll(num, test_mode);
    else
        return test_scroll(256, FALSE);
}
