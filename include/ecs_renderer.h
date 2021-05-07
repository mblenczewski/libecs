#ifndef ECS_RENDERER
#define ECS_RENDERER

#include "ecs.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

struct ecs_renderer;

extern b8 ecs_renderer_try_alloc(u32 width, u32 height, GLFWwindow *window, struct ecs_renderer **out);
extern void ecs_renderer_free(struct ecs_renderer *renderer);

extern void ecs_renderer_render_frame(struct ecs_renderer *renderer);

struct ecs_vertex {
	vec2 pos;
	vec3 colour;
};

struct ecs_uniform_buffer_object {
	_Alignas(16) mat4 model;
	_Alignas(16) mat4 view;
	_Alignas(16) mat4 proj;
};

#endif // ECS_RENDERER
