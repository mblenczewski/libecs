#include "ecs.h"
#include "ecs_memory.h"

#include "test.h"

#define TYPE u8

#define CAPACITY 1000 
#define SIZE sizeof(TYPE) * CAPACITY

int test_create(void) {
	struct ecs_memory_arena *arena = NULL;
	b8 created = ecs_memory_arena_try_create(sizeof(TYPE), SIZE, &arena);
	ECS_ASSERT(created, "Could not allocate arena!");

	ecs_memory_arena_destroy(arena);
	PASS();
}

int test_alloc(void) {
	struct ecs_memory_arena *arena = NULL;
	b8 created = ecs_memory_arena_try_create(sizeof(TYPE), SIZE, &arena);
	ECS_ASSERT(created, "Could not allocate arena!");

	TYPE *value = NULL;
	b8 allocated_single = ecs_memory_arena_try_alloc(arena, sizeof(TYPE), (void*)&value);
	ECS_ASSERT(allocated_single, "Could not allocate single value!");
	ECS_ASSERT(value, "Allocated pointer is null!");

	*value = 42;
	ECS_ASSERT(*value == 42, "Can not mutate allocated value!");

	TYPE *values = NULL;
	b8 allocated_many = ecs_memory_arena_try_alloc(arena, (CAPACITY - 1) * sizeof(TYPE), (void*)&values);
	ECS_ASSERT(allocated_many, "Could not allocate many values!");
	ECS_ASSERT(values, "Allocated pointer is null!");

	ECS_ASSERT(*value == *(values - sizeof(TYPE)), "Did not allocate values contiguously!");

	TYPE overflow;
	b8 allocated_overflow = ecs_memory_arena_try_alloc(arena, sizeof(TYPE), (void*)&overflow);
	ECS_ASSERT_NE(allocated_overflow, "Invalid allocation overflowed arena!");

	ecs_memory_arena_destroy(arena);
	PASS();
}

int main(void) {
	TEST_BEGIN();

	TEST_RUN(test_create);
	TEST_RUN(test_alloc);

	TEST_END();
}

