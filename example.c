#include "ecs.h"
#include "version.h"

#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cglm/vec4.h>
#include <cglm/mat4.h>


int main(int argc, char **argv) {
	printf("(libecs %s) Hello, World!\n", ECS_VERSION);

	for (u32 i = 0; i < (u32)argc; i++) {
		printf("Arg %d: %s\n", i, argv[i]);
	}

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(800, 600, "libecs test", NULL, NULL);

	u32 extension_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	printf("[Vk] %d extensions supported\n", extension_count);

	mat4 matrix = {0};
	vec4 vector = {0}, out = {0};
	glm_mat4_mulv(matrix, vector, out);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

