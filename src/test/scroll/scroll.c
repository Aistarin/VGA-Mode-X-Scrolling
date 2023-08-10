#include "scroll.h"
#include "src/common.h"
#include "src/ecs/ecs.h"
#include "src/gfx/gfx.h"
#include "src/gfx/spr.h"
#include "src/gfx/vga.h"
#include "src/io/bitmap.h"
#include "src/io/file.h"
#include "src/io/keyboard.h"
#include "src/io/timer.h"
#include "src/test/pattern.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool exit_program = FALSE;
byte palette[256*3];
byte scratch_buffer[0xFFFF];

int view_pos_x = 0;
int view_pos_y = 0;

gfx_buffer *tileset_buffer;
gfx_tilemap *tilemap_buffer;

int speed_multiplier = 0;
int timer = 0;
int max_entities = 0;
void *drawable;

void* load_sprite(char *filename, word sprite_width, word sprite_height, bool compiled, byte palette_offset) {
    gfx_buffer *sprite_buffer;
    dword compiled_sized = 0;

    load_bmp_to_buffer("jodi-spr.bmp", scratch_buffer, sprite_width, sprite_height, palette, palette_offset);

    if(compiled) {
        compiled_sized = spr_compile_planar_sprite_scheme_2(scratch_buffer, sprite_width, sprite_height, NULL, NULL, palette_offset);
        sprite_buffer = gfx_create_empty_buffer(0, sprite_width, sprite_height, TRUE, compiled_sized);
        spr_compile_planar_sprite_scheme_2(scratch_buffer, sprite_width, sprite_height, sprite_buffer->buffer, sprite_buffer->plane_offsets, palette_offset);
        sprite_buffer->buffer_flags |= GFX_BUFFER_FLAG_CLIPPING;
    } else {
        sprite_buffer = gfx_create_empty_buffer(0, sprite_width, sprite_height, TRUE, 0);
        gfx_load_linear_bitmap_to_planar_bitmap(scratch_buffer, sprite_buffer->buffer, sprite_width, sprite_height, TRUE);
    }

    return sprite_buffer;
}

void entity_draw_handler(ecs_entity *entity, void *component) {
    ecs_component_drawable *component_drawable = (ecs_component_drawable *) component;
    ecs_component_position *component_position = entity->components[ECS_COMPONENT_TYPE_POSITION];

    int draw_x = component_position->x + component_drawable->x_offset;
    int draw_y = component_position->y + component_drawable->y_offset;

    gfx_draw_bitmap_to_screen((gfx_buffer *) component_drawable->drawable, draw_x, draw_y, FALSE);
}

void entity_movement_handler(ecs_entity *entity, void *component) {
    ecs_component_movement *component_movement = (ecs_component_movement *) component;
    ecs_component_position *component_position = entity->components[ECS_COMPONENT_TYPE_POSITION];

    component_position->x += component_movement->hspeed;
    component_position->y += component_movement->vspeed;

    if(component_position->x > (320 - 32) || component_position->x < 0) {
        if(component_position->x > (320 - 32)) component_position->x = (320 - 32);
        else if(component_position->x < 0) component_position->x = 0;
        component_movement->hspeed = -(component_movement->hspeed);
    }

    if(component_position->y > (240 - 56) || component_position->y < 0) {
        if(component_position->y > (240 - 56)) component_position->y = (240 - 56);
        else if(component_position->y < 0) component_position->y = 0;
        component_movement->vspeed = -(component_movement->vspeed);
    }
}

ecs_entity* create_entity(gfx_buffer *drawable) {
    ecs_entity *entity = ecs_instantiate_empty_entity(ECS_ENTITY_TYPE_SPRITE);
    ecs_component_position *component_position = (ecs_component_position *) ecs_attach_component_to_entity(entity, ECS_COMPONENT_TYPE_POSITION);
    ecs_component_drawable *component_drawable = (ecs_component_drawable *) ecs_attach_component_to_entity(entity, ECS_COMPONENT_TYPE_DRAWABLE);
    ecs_component_movement *component_movement = (ecs_component_movement *) ecs_attach_component_to_entity(entity, ECS_COMPONENT_TYPE_MOVEMENT);

    component_position->x = rand() % (SCREEN_WIDTH - component_drawable->width);
    component_position->y = rand() % (SCREEN_HEIGHT - component_drawable->height);
    component_drawable->drawable = drawable;
    component_drawable->width = drawable->width;
    component_drawable->height = drawable->height;
    component_movement->hspeed = 1 + rand() % 5;
    component_movement->vspeed = 1 + rand() % 5;
    return entity;
}

int shutdown_handler(void) {
    ecs_shutdown();
    timer_shutdown();
    keyboard_shutdown();
    gfx_shutdown();

    return 0;
}

int init_handler(void) {
    ecs_init();
    timer_init();
    keyboard_init();
    gfx_init();

    ecs_register_component(ECS_COMPONENT_TYPE_POSITION, sizeof(ecs_component_position), ENTITY_MAX, NULL, FALSE);
    ecs_register_component(ECS_COMPONENT_TYPE_DRAWABLE, sizeof(ecs_component_drawable), ENTITY_MAX, entity_draw_handler, TRUE);
    ecs_register_component(ECS_COMPONENT_TYPE_MOVEMENT, sizeof(ecs_component_movement), ENTITY_MAX, entity_movement_handler, FALSE);

    return 0;
}

void handle_input() {
    speed_multiplier = !is_pressing_lshift();

    if(is_pressing_w()) {
        view_pos_y -= speed_multiplier ? 1 : TILE_HEIGHT;
    }
    if(is_pressing_s()) {
        view_pos_y += speed_multiplier ? 1 : TILE_HEIGHT;
    }
    if(is_pressing_a()) {
        view_pos_x -= speed_multiplier ? 1 : TILE_WIDTH;
    }
    if(is_pressing_d()) {
        view_pos_x += speed_multiplier ? 1 : TILE_WIDTH;
    }
    if(is_pressing_escape()) {
        exit_program = TRUE;
    }

    if(view_pos_x < 0)
        view_pos_x = 0;
    if(view_pos_y < 0)
        view_pos_y = 0;
}

void handle_logic() {
    if(timer++ % 30 == 0 && ecs_get_entity_count() < max_entities){
        create_entity(drawable);
    }
    ecs_handle_components();
}

void handle_graphics() {
    /* scroll screen before rendering all */
    gfx_set_scroll_offset(view_pos_x, view_pos_y);
    ecs_handle_component_type(ECS_COMPONENT_TYPE_DRAWABLE);
    gfx_render_all();
}

int test_scroll(byte test_mode){
    int x, y, i;
    word render_tile_width = PAGE_WIDTH / TILE_WIDTH;
    word render_tile_height = PAGE_HEIGHT / TILE_HEIGHT;

    drawable = load_sprite("jodi-spr.bmp", 32, 56, TRUE, 0);

    init_handler();

    tileset_buffer = gfx_get_tileset_buffer();
    tilemap_buffer = gfx_get_tilemap_buffer();

    /* load or generate tileset images */
    if(test_mode == 3) {
        load_bmp_to_buffer("jodi.bmp", tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height, palette, 0);
        vga_set_palette(palette, 0, 255);
    } else if (test_mode == 2) {
        render_pattern_to_buffer_3(tileset_buffer->buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    } else if (test_mode == 1) {
        render_pattern_to_buffer_2(tileset_buffer->buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    } else if (test_mode == 0) {
        render_pattern_to_buffer_1(tileset_buffer->buffer, TILE_WIDTH, TILE_HEIGHT, tileset_buffer->width, tileset_buffer->height);
    }
    gfx_load_tileset();

    /* build test tilemap */
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

    palette[0] = 0;
    palette[1] = 0;
    palette[2] = 0;
    vga_set_palette(palette, 0, 15);

    timer_set_interval(16);
    while (1) {
        timer_start();

        while(timer_step()) {
            handle_input();
            handle_logic();
        }

        handle_graphics();

        timer_end();

        if(exit_program) {
            break;
        }
    }

    shutdown_handler();

    printf("total objects rendered: %d\n", ecs_get_entity_count());

    return 0;
}

int main(int argc, char *argv[]) {
    char *a = argv[1];
    char *b = argv[2];
    int num = atoi(a);
    int test_mode = atoi(b);

    if(argc >= 2) {
        max_entities = num;
        return test_scroll(test_mode);
    }
    else {
        max_entities = ENTITY_MAX;
        return test_scroll(0);
    }
}
