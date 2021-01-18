#ifndef ECS_SYSTEM_H
#define ECS_SYSTEM_H

#include "ecs.h"

enum ecs_system_errno {
	ECS_SYSTEM_OK,
	ECS_SYSTEM_PAUSE,
	ECS_SYSTEM_STOP,
};

typedef b8 (*ecs_system_setup)();
typedef void (*ecs_system_teardown)();

struct ecs_system {
	char *name;
	ecs_system_setup setup;
	ecs_system_teardown teardown; 
};

#endif // ECS_SYSTEM_H

