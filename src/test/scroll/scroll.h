#ifndef SCROLL_H_
#define SCROLL_H_

#include "src/common.h"

enum ecs_entity_types {
    ECS_ENTITY_TYPE_NULL,                       // Entity type 0 is reserved by the system
    ECS_ENTITY_TYPE_SPRITE
};

enum ecs_component_types {
    ECS_COMPONENT_TYPE_POSITION,
    ECS_COMPONENT_TYPE_MOVEMENT,
    ECS_COMPONENT_TYPE_DRAWABLE
};

typedef struct ecs_component_position {
    byte entity_id;
    int x;
    int y;
} ecs_component_position;

typedef struct ecs_component_movement {
    byte entity_id;
    int hspeed;
    int vspeed;
} ecs_component_movement;

typedef struct ecs_component_drawable {
    byte entity_id;
    int x_offset;
    int y_offset;
    word width;
    word height;
    byte palette_offset;
    void *drawable;
} ecs_component_drawable;

#endif
