#ifndef ECS_MEMORY_H
#define ECS_MEMORY_H

#include "ecs.h"

struct ecs_memory_arena;

b8 ecs_memory_arena_try_create(usize alignment, usize size, struct ecs_memory_arena **arena);
void ecs_memory_arena_destroy(struct ecs_memory_arena *arena);

b8 ecs_memory_arena_try_alloc(struct ecs_memory_arena *arena, usize size, void **out);

#endif // ECS_MEMORY_H

