#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "src/common.h"

enum ecs_entity_types {
    ECS_ENTITY_TYPE_NULL,                       // Entity type 0 is reserved by the system
    ECS_ENTITY_TYPE_PLAYER,
    ECS_ENTITY_TYPE_ENEMY,
    ECS_ENTITY_TYPE_COIN
};

enum ecs_component_types {
    ECS_COMPONENT_TYPE_NULL,
    ECS_COMPONENT_TYPE_POSITION,
    ECS_COMPONENT_TYPE_PHYSICS,
    ECS_COMPONENT_TYPE_TILE_COLLISION,
    ECS_COMPONENT_TYPE_ENTITY_COLLISION,
    ECS_COMPONENT_TYPE_DRAWABLE
};

typedef struct ecs_component_position {
    byte entity_id;
    int x;
    int y;
    int x_previous;
    int y_previous;
} ecs_component_position;

typedef struct ecs_component_drawable {
    byte entity_id;
    int x_offset;
    int y_offset;
    word width;
    word height;
    bool display;
    bool flip_horz;
    void *drawable;
} ecs_component_drawable;

typedef struct ecs_component_physics {
    byte entity_id;
    int hspeed;
    int vspeed;
    int friction;
    int gravity;
} ecs_component_physics;

#endif
