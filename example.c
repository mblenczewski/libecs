#include "ecs.h"
#include "ecs_version.h"
#include "ecs_renderer.h"

#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/vec4.h>
#include <cglm/mat4.h>


const u32 HEIGHT = 600;
const u32 WIDTH = 800;
const char *NAME = "libecs example";

int main(int argc, char **argv) {
	printf("(libecs %s) Hello, World!\n", ECS_VERSION);

	for (u32 i = 0; i < (u32)argc; i++) {
		printf("Arg %d: %s\n", i, argv[i]);
	}

	/* cglm test */
	mat4 matrix = {0};
	vec4 vector = {0}, out = {0};
	glm_mat4_mulv(matrix, vector, out);

	glfwInit();

	u32 instance_extension_count;
	vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
	VkExtensionProperties *instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
	vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);

	for (usize i = 0; i < instance_extension_count; i++) {
		printf("Found instance extension: %s\n", instance_extensions[i].extensionName);
	}

	u32 instance_layer_count;
	vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
	VkLayerProperties *instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
	vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);

	for (usize i = 0; i < instance_layer_count; i++) {
		printf("Found instance layer: %s\n", instance_layers[i].layerName);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, NAME, NULL, NULL);

	struct ecs_renderer *renderer = NULL;
	b8 did_setup = ecs_renderer_try_setup(WIDTH, HEIGHT, window, &renderer);
	ECS_ASSERT(did_setup, "Could not setup renderer!");
	ECS_ASSERT(renderer, "Renderer pointer was null!");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ecs_renderer_render_frame(renderer);
	}

	ecs_renderer_teardown(renderer);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

