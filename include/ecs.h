#ifndef ECS_ECS_H
#define ECS_ECS_H

#include "ecs_version.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Integral Types */
typedef uint8_t b8;

typedef size_t usize;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

/* Assertions */
#ifndef NDEBUG

#define __ECS_ASSERT_IMPL(condition, message, file, line) ((void)0)
#define __ECS_ASSERT_NE_IMPL(condition, message, file, line) ((void)0)

#else // NDEBUG

#define __ECS_ASSERT_IMPL(condition, message, file, line, func) \
{ \
	if (condition) { } else { \
		printf("(%s:%d) %s: %s - %s\n", file, line, func, #condition, message); \
		exit(1); \
	} \
}

#define __ECS_ASSERT_NE_IMPL(condition, message, file, line, func) \
{ \
	if (condition) { \
		printf("(%s:%d) %s: %s - %s\n", file, line, func, #condition, message); \
		exit(1); \
	} \
}

#endif // NDEBUG

#define ECS_ASSERT(condition, message) \
	__ECS_ASSERT_IMPL(condition, message, __FILE__, __LINE__, __func__)

#define ECS_ASSERT_NE(condition, message) \
	__ECS_ASSERT_NE_IMPL(condition, message, __FILE__, __LINE__, __func__)

#endif // ECS_ECS_H

