#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int entity_count = 0;
ecs_entity *entity_list;

ecs_component_list *component_collection[COMPONENT_MAX];
ecs_component_list *component_position_list;
ecs_component_list *component_drawable_list;
ecs_component_list *component_physics_list;

ecs_entity* ecs_instantiate_empty_entity(byte entity_type) {
    int i;
    ecs_entity *entity = &entity_list[++entity_count];
    entity->entity_id = entity_count;
    entity->entity_type = entity_type;
    entity->component_count = 0;

    // (re-)initialize component list
    for(i = 0; i < COMPONENT_MAX; i++) {
        entity->components[i] = NULL;
    }

    return entity;
}

ecs_entity* _get_entity_by_id(byte entity_id) {
    return &entity_list[entity_id];
}

void* _get_component_by_entity_id(byte entity_id, byte component_type) {
    ecs_entity *entity = _get_entity_by_id(entity_id);
    return entity->components[component_type];
}

ecs_component_list* _init_component_list(byte component_type, byte component_size, byte component_count_max) {
    struct ecs_component_list* component_list;
    dword array_size = sizeof(byte) * (dword) component_size * component_count_max;

    component_list = malloc(sizeof(struct ecs_component_list) + array_size);

    component_list->component_type = component_type;
    component_list->component_size = component_size;
    component_list->component_count_max = component_count_max;
    component_list->component_count = 0;

    /* component list array is memory immediately after main struct */
    component_list->components = (byte *) component_list + sizeof(struct ecs_component_list);

    memset((byte *) component_list->components, 0, array_size);

    return component_list;
}

void _handle_physics_components() {
    int i;
    ecs_component_physics *component_list = component_physics_list->components;
    ecs_component_physics *component_physics;
    ecs_component_position *component_position;

    for(i = 0; i < COMPONENT_MAX; i++) {
        component_physics = &component_list[i];
        if(component_physics->entity_id == 0) {
            continue;
        }
        component_position = (ecs_component_position *) _get_component_by_entity_id(
            component_physics->entity_id,
            ECS_COMPONENT_TYPE_POSITION
        );
        component_physics->hspeed -= component_physics->friction;
        component_physics->vspeed -= component_physics->gravity;
        component_position->x += component_physics->hspeed;
        component_position->y += component_physics->vspeed;
    }
}

void ecs_init(void) {
    entity_list = malloc(sizeof(ecs_entity) * ENTITY_MAX);
    component_position_list = _init_component_list(ECS_COMPONENT_TYPE_POSITION, sizeof(ecs_component_position), ENTITY_MAX);
    component_drawable_list = _init_component_list(ECS_COMPONENT_TYPE_DRAWABLE, sizeof(ecs_component_position), ENTITY_MAX);
    component_physics_list = _init_component_list(ECS_COMPONENT_TYPE_PHYSICS, sizeof(ecs_component_position), ENTITY_MAX);

    component_collection[ECS_COMPONENT_TYPE_NULL] = NULL;
    component_collection[ECS_COMPONENT_TYPE_POSITION] = component_position_list;
    component_collection[ECS_COMPONENT_TYPE_DRAWABLE] = component_drawable_list;
    component_collection[ECS_COMPONENT_TYPE_PHYSICS] = component_physics_list;
}

void ecs_shutdown(void) {
    // TODO: free memory
}
