#include "ecs.h"
#include "ecs_memory.h"

struct ecs_memory_arena {
	usize offset; // offset of the current "head" pointer from the base
	usize length; // the number of bytes of backing memory in the arena
	u8 *base; // pointer to the backing memory of the arena
	// TODO: is there a way of converting "base" from a pointer to a flexible length array member?
};

b8 ecs_memory_arena_try_create(usize alignment, usize size, struct ecs_memory_arena **arena) {
	ECS_ASSERT(size % alignment == 0, "Given size must be a multiple of the given alignment!");
	ECS_ASSERT(arena, "Given arena out pointer is null!");
	ECS_ASSERT_NE(*arena, "Given arena out pointer points to non-null value (would overwrite existing arena)!");

	*arena = malloc(sizeof(struct ecs_memory_arena));
	if (*arena) {
		(*arena)->offset = 0;
		(*arena)->length = size;

		u8 *arena_memory = aligned_alloc(alignment, size);
		if (arena_memory) {
			(*arena)->base = arena_memory;
			return true;
		}

		free(arena_memory);
	}

	free(*arena);
	*arena = NULL;

	return false;
}

void ecs_memory_arena_destroy(struct ecs_memory_arena *arena) {
	ECS_ASSERT(arena, "Given arena pointer is null!");

	free(arena->base);
	free(arena);
}

b8 ecs_memory_arena_try_alloc(struct ecs_memory_arena *arena, usize size, void **out) {
	ECS_ASSERT(arena, "Given arena pointer is null!");
	ECS_ASSERT(size > 0, "Given allocation size must be greater than 0!");

	if (arena->offset + size <= arena->length) {
		*out = arena->base + arena->offset;

		arena->offset += size;

		return true;
	}

	return false;
}

