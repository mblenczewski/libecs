#ifndef ECS_SYSTEM_H
#define ECS_SYSTEM_H

#include "ecs.h"
#include "ecs_component.h"

typedef u64 sid_t;

enum ecs_system_errno {
	ECS_SYSTEM_OK,
	ECS_SYSTEM_PAUSE,
	ECS_SYSTEM_STOP,
};

typedef b8 (*ecs_system_setup)();
typedef void (*ecs_system_teardown)();

#define ECS_SYSTEM_MAX_ACTIONS 8

struct ecs_system_action {
	usize archetype_length;
	cid_t *archetype;
	enum ecs_system_errno(*delegate)();
};

struct ecs_system;

#endif // ECS_SYSTEM_H

