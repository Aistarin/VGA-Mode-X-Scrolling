#ifndef ECS_H_
#define ECS_H_

#include "src/common.h"

#define ENTITY_MAX 255
#define COMPONENT_MAX 16

enum ecs_entity_types {
    ECS_ENTITY_TYPE_NULL,
    ECS_ENTITY_TYPE_PLAYER,
    ECS_ENTITY_TYPE_ENEMY,
    ECS_ENTITY_TYPE_COIN
};

enum ecs_component_types {
    ECS_COMPONENT_TYPE_NULL,
    ECS_COMPONENT_TYPE_POSITION,
    ECS_COMPONENT_TYPE_DRAWABLE,
    ECS_COMPONENT_TYPE_PHYSICS,
    ECS_COMPONENT_TYPE_TILE_COLLISION,
    ECS_COMPONENT_TYPE_ENTITY_COLLISION
};

typedef struct ecs_entity {
    byte entity_id;
    byte entity_type;
    byte component_count;
    void *components[COMPONENT_MAX];            // max components each entity can have attached to it
} ecs_entity;

typedef struct ecs_entity_list {
    byte entity_count;
    ecs_entity entities[ENTITY_MAX];
} ecs_entity_list;

typedef struct ecs_component_list {
    byte component_type;
    byte component_size;
    byte component_count;
    byte component_count_max;
    void *components;
} ecs_component_list;

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

void ecs_init(void);
void ecs_handle(void);
void ecs_shutdown(void);

void ecs_set_drawing_function(void (*func)(ecs_entity*));
ecs_entity* ecs_instantiate_empty_entity(byte entity_type);
void* ecs_attach_component_to_entity(ecs_entity *entity, byte component_type);

#endif
