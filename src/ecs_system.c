#include "ecs.h"
#include "ecs_system.h"

struct ecs_system {
	char *name;
	void *state;
	ecs_system_setup setup;
	ecs_system_teardown teardown;
	struct ecs_system_action actions[ECS_SYSTEM_MAX_ACTIONS];
};
