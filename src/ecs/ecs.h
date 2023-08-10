#ifndef ECS_H_
#define ECS_H_

#include "src/common.h"

#define ENTITY_MAX 256
#define COMPONENT_MAX 16

typedef struct ecs_entity {
    byte entity_id;
    byte entity_type;
    int component_count;
    void *components[COMPONENT_MAX];            // max components each entity can have attached to it
} ecs_entity;

typedef struct ecs_component_list {
    byte component_type;
    byte component_size;
    int component_count;
    int component_count_max;
    bool handle_manually;                       // skip this component in the loop
    void (*handler)(ecs_entity*, void*);
    void *components;
} ecs_component_list;

void ecs_init(void);
void ecs_handle_component_type(byte component_type);
void ecs_handle_components(void);
void ecs_shutdown(void);

int ecs_get_entity_count(void);

ecs_entity* ecs_instantiate_empty_entity(byte entity_type);
void* ecs_attach_component_to_entity(ecs_entity *entity, byte component_type);

ecs_component_list* ecs_register_component(byte component_type, int component_size, int component_count_max, void (*handler)(ecs_entity*, void*), bool handle_manually);

#endif
