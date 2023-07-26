#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ecs_entity_list *entity_list;
ecs_component_list *component_collection[COMPONENT_MAX];
ecs_component_list *component_position_list;
ecs_component_list *component_drawable_list;
ecs_component_list *component_physics_list;

void (*draw_func)(ecs_entity*);

void ecs_set_drawing_function(void (*func)(ecs_entity*)) {
    draw_func = func;
}

void* _get_component_at_position(ecs_component_list* component_list, int pos) {
    byte *component;
    byte *components = (byte *) component_list->components;
    return components + (pos * component_list->component_size);
}

ecs_entity* ecs_instantiate_empty_entity(byte entity_type) {
    int i;
    ecs_entity *entity;

    // scan entity list until we find an "empty" one represented by a null entity type,
    // skipping entity 0 (since it is reserved as a null entity value for components)
    for(i = 1; i < ENTITY_MAX; i++) {
        entity = &entity_list->entities[i];
        if(entity->entity_type == ECS_ENTITY_TYPE_NULL)
            break;
    }

    entity->entity_id = ++entity_list->entity_count;
    entity->entity_type = entity_type;
    entity->component_count = 0;

    // (re-)initialize component list
    for(i = 0; i < COMPONENT_MAX; i++) {
        entity->components[i] = NULL;
    }

    printf("Instantiated entity of type %d with id %d\n", entity->entity_type, entity->entity_id);
    return entity;
}

void* ecs_attach_component_to_entity(ecs_entity *entity, byte component_type) {
    int i;
    byte *component;
    byte entity_id;
    ecs_component_list *component_list = component_collection[component_type];

    if(component_list->component_count == component_list->component_count_max) {
        printf("Maximum component count for type %d reached: %d", component_type, component_list->component_count);
        return NULL;
    }

    for(i = 0; i < component_list->component_count_max; i++) {
        component = _get_component_at_position(component_list, i);
        /* NOTE: since entity_id is the first item defined in component
         * structs, we can simply cast the component pointer as a byte
         * array and read the first element to obtain entity_id without
         * need to read 2 or 4 consecutive bytes of unknown endianness
         */
        entity_id = ((byte *) component)[0];

        if(entity_id == 0) {
            memset(component, 0, component_list->component_size);
            component[0] = entity->entity_id;
            entity->components[component_type] = component;
            component_list->component_count++;
            printf("Attached component %d to entity %d\n", component_type, entity->entity_id);
            return component;
        }
    }

    return NULL;
}

ecs_entity* _get_entity_by_id(byte entity_id) {
    return &entity_list->entities[entity_id];
}

void* _get_component_by_entity_id(byte entity_id, byte component_type) {
    ecs_entity *entity = _get_entity_by_id(entity_id);
    return entity->components[component_type];
}

ecs_entity_list* _init_entity_list() {
    struct ecs_entity_list* new_entity_list;
    new_entity_list = malloc(sizeof(struct ecs_entity_list));
    memset((byte *) new_entity_list, 0, sizeof(struct ecs_entity_list));
    return new_entity_list;
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

    printf("Instantiating component type %d with max count %d\n", component_list->component_type, component_list->component_count_max);
    return component_list;
}

void _handle_drawable_components() {
    int i;
    ecs_component_drawable *component_drawable;

    for(i = 0; i < component_drawable_list->component_count_max; i++) {
        component_drawable = _get_component_at_position(component_drawable_list, i);
        // skip if entity is null or display flag is set to false
        if(component_drawable->entity_id == 0 || component_drawable->display == FALSE) {
            continue;
        }
        draw_func(_get_entity_by_id(component_drawable->entity_id));
    }
}

void _handle_physics_components() {
    int i;
    ecs_component_physics *component_physics;
    ecs_component_position *component_position;

    for(i = 0; i < component_physics_list->component_count_max; i++) {
        component_physics = _get_component_at_position(component_physics_list, i);
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

void ecs_handle(void) {
    _handle_physics_components();
    _handle_drawable_components();
}

void ecs_init(void) {
    entity_list = _init_entity_list();
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
