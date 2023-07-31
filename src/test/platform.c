#include "src/common.h"
#include "src/ecs/ecs.h"
#include "src/gfx/gfx.h"
#include "src/gfx/spr.h"
#include "src/gfx/vga.h"
#include "src/io/bitmap.h"
#include "src/io/file.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

bool exit_program = FALSE;
byte palette[256*3];
byte scratch_buffer[0xFFFF];

int view_pos_x = 0;
int view_pos_y = 0;

gfx_buffer *tileset_buffer;
gfx_tilemap *tilemap_buffer;

void* load_sprite(char *filename, word sprite_width, word sprite_height, bool compiled) {
    gfx_buffer *sprite_buffer;
    dword compiled_sized = 0;

    load_bmp_to_buffer("jodi-spr.bmp", scratch_buffer, sprite_width, sprite_height, palette);

    if(compiled) {
        compiled_sized = spr_compile_planar_sprite_scheme_2(scratch_buffer, sprite_width, sprite_height, NULL, NULL);
        sprite_buffer = gfx_create_empty_buffer(0, sprite_width, sprite_height, TRUE, compiled_sized);
        spr_compile_planar_sprite_scheme_2(scratch_buffer, sprite_width, sprite_height, sprite_buffer->buffer, sprite_buffer->plane_offsets);
    } else {
        sprite_buffer = gfx_create_empty_buffer(0, sprite_width, sprite_height, TRUE, 0);
        gfx_load_linear_bitmap_to_planar_bitmap(scratch_buffer, sprite_buffer->buffer, sprite_width, sprite_height, TRUE);
    }

    return sprite_buffer;
}

void draw_entity(ecs_entity *entity) {
    ecs_component_position *component_position = entity->components[ECS_COMPONENT_TYPE_POSITION];
    ecs_component_drawable *component_drawable = entity->components[ECS_COMPONENT_TYPE_DRAWABLE];
    int draw_x = component_position->x + component_drawable->x_offset;
    int draw_y = component_position->y + component_drawable->y_offset;
    int x_offset = 0, y_offset = 0;

    // do not draw sprites that go beyond boundaries
    if((draw_x + component_drawable->width) < view_pos_x
        || draw_x > view_pos_x + SCREEN_WIDTH
        || (draw_y + component_drawable->height) < view_pos_y
        || draw_y > view_pos_y + SCREEN_HEIGHT) {
        return;
    }

    if(draw_x < view_pos_x) {
        x_offset = (view_pos_x - draw_x) * -1;
        draw_x = view_pos_x;
    } else if(draw_x + component_drawable->width > view_pos_x + SCREEN_WIDTH) {
        x_offset = component_drawable->width - (SCREEN_WIDTH - draw_x);
        draw_x = view_pos_x + SCREEN_WIDTH - component_drawable->width;
    }

    if(draw_y < view_pos_y) {
        y_offset = (view_pos_y - draw_y) * -1;
        draw_y = view_pos_y;
    } else if(draw_y + component_drawable->height > view_pos_y + SCREEN_HEIGHT) {
        y_offset = component_drawable->height - (SCREEN_HEIGHT - draw_y);
        draw_y = view_pos_y + SCREEN_HEIGHT - component_drawable->height;
    }

    gfx_draw_sprite_to_screen(
        (gfx_buffer *) component_drawable->drawable,
        0,
        0,
        (word) draw_x,
        (word) draw_y,
        component_drawable->width,
        component_drawable->height,
        x_offset,
        y_offset,
        component_drawable->flip_horz
    );
}

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
    ecs_handle();
}

void handle_graphics() {
    /* scroll screen before rendering all */
    gfx_set_scroll_offset(view_pos_x, view_pos_y);
    gfx_render_all();
}

int main(int argc, char *argv[]) {
    ecs_entity *player;
    ecs_component_position *component_position;
    ecs_component_drawable *component_drawable;
    ecs_component_physics *component_physics;
    void *drawable;

    ecs_init();
    gfx_init();

    ecs_set_drawing_function(draw_entity);

    drawable = load_sprite("jodi-spr.bmp", 32, 56, TRUE);

    // tileset_buffer = gfx_get_tileset_buffer();
    // tilemap_buffer = gfx_get_tilemap_buffer();

    // load_bmp_to_buffer("testtile.bmp", tileset_buffer->buffer, tileset_buffer->width, tileset_buffer->height, palette);
    vga_set_palette(palette, 0, 255);
    // gfx_load_tileset();

    // read_bytes_from_file("test.map", tilemap_buffer->buffer, tilemap_buffer->buffer_size);
    // gfx_reload_tilemap(0, 0);

    player = ecs_instantiate_empty_entity(ECS_ENTITY_TYPE_PLAYER);
    component_position = (ecs_component_position *) ecs_attach_component_to_entity(player, ECS_COMPONENT_TYPE_POSITION);
    component_drawable = (ecs_component_drawable *) ecs_attach_component_to_entity(player, ECS_COMPONENT_TYPE_DRAWABLE);
    component_physics = (ecs_component_physics *) ecs_attach_component_to_entity(player, ECS_COMPONENT_TYPE_PHYSICS);

    component_position->x = 0;
    component_position->y = 0;
    component_drawable->display = TRUE;
    component_drawable->flip_horz = FALSE;
    component_drawable->drawable = drawable;
    component_drawable->width = 32;
    component_drawable->height = 56;
    component_physics->hspeed = 1;
    component_physics->vspeed = 1;

    player = ecs_instantiate_empty_entity(ECS_ENTITY_TYPE_PLAYER);
    component_position = (ecs_component_position *) ecs_attach_component_to_entity(player, ECS_COMPONENT_TYPE_POSITION);
    component_drawable = (ecs_component_drawable *) ecs_attach_component_to_entity(player, ECS_COMPONENT_TYPE_DRAWABLE);
    component_physics = (ecs_component_physics *) ecs_attach_component_to_entity(player, ECS_COMPONENT_TYPE_PHYSICS);

    component_position->x = 288;
    component_position->y = 0;
    component_drawable->display = TRUE;
    component_drawable->flip_horz = FALSE;
    component_drawable->drawable = drawable;
    component_drawable->width = 32;
    component_drawable->height = 56;
    component_physics->hspeed = -1;
    component_physics->vspeed = 1;

    while(!exit_program) {
        handle_input();
        handle_logic();
        handle_graphics();
    }

    ecs_shutdown();
    gfx_shutdown();

    return 0;
}
