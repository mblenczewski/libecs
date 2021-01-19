#include "ecs.h"
#include "ecs_version.h"
#include "ecs_renderer.h"

#include <string.h>

const VkAllocationCallbacks *vk_allocator = NULL;

const usize VALIDATION_LAYER_COUNT = 1;
const char *VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
const b8 ENABLED_VALIDATION_LAYERS = false;
#else // NDEBUG
const b8 ENABLED_VALIDATION_LAYERS = true;
#endif // NDEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL vk_layer_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data) {
	printf("%s\n", callback_data->pMessage);

	return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT vk_get_debug_utils_messenger_create_info() {
	VkDebugUtilsMessageSeverityFlagsEXT severity = 0;
	severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	VkDebugUtilsMessageTypeFlagsEXT type = 0;
	type |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
	type |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	type |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = severity,
		.messageType = type,
		.pfnUserCallback = vk_layer_debug_callback,
		.pUserData = NULL, /* optional parameter */
	};

	return messenger_create_info;
}

static VkResult vk_create_debug_utils_messenger(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT *create_info,
		const VkAllocationCallbacks *allocator,
		VkDebugUtilsMessengerEXT *callback) {
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func)
		return func(instance, create_info, allocator, callback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void vk_destroy_debug_utils_messenger(
		VkInstance instance,
		VkDebugUtilsMessengerEXT callback,
		const VkAllocationCallbacks *allocator) {
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func)
		func(instance, callback, allocator);
}

static b8 vk_instance_supports_layers(usize count, const char **layers) {
	u32 instance_layer_count = 0;
	vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);

	VkLayerProperties *instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
	ECS_ASSERT(instance_layers, "Could not allocate memory for VkInstance layer properties!");
	vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);

	for (usize i = 0; i < count; i++) {
		const char *required_layer = layers[i];

		b8 layer_found = false;
		for (usize j = 0; j < instance_layer_count; j++) {
			VkLayerProperties layer = instance_layers[j];

			if (strcmp(required_layer, layer.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			free(instance_layers);
			return false;
		}
	}

	free(instance_layers);
	return true;
}

static b8 vk_instance_supports_extensions(usize count, const char **extensions) {
	u32 instance_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);

	VkExtensionProperties *instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
	ECS_ASSERT(instance_extensions, "Could not allocate memory for VkInstance extension properties!");
	vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);

	for (usize i = 0; i < count; i++) {
		const char *required_extension = extensions[i];

		b8 extension_found = false;
		for (usize j = 0; j < instance_extension_count; j++) {
			VkExtensionProperties extension = instance_extensions[j];

			if (strcmp(required_extension, extension.extensionName) == 0) {
				extension_found = true;
				break;
			}
		}

		if (!extension_found) {
			free(instance_extensions);
			return false;	
		}
	}

	free(instance_extensions);
	return true;
}

struct ecs_renderer {
	u32 width, height;
	GLFWwindow *window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
};

static b8 ecs_renderer_try_create_instance(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_setup_messenger(struct ecs_renderer *renderer);

b8 ecs_renderer_try_setup(u32 width, u32 height, GLFWwindow *window, struct ecs_renderer **out) {
	ECS_ASSERT(window, "Given GLFW window pointer is null!");
	ECS_ASSERT(out, "Given out pointer is null!");
	ECS_ASSERT_NE(*out, "Given out pointer points to non-null value (would overwrite existing renderer)!");

	*out = malloc(sizeof(struct ecs_renderer));
	if (*out) {
		(*out)->width = width;
		(*out)->height = height;
		(*out)->window = window;

		b8 succeeded = false;
		
		succeeded = ecs_renderer_try_create_instance(*out);
		if (!succeeded)
			goto cleanup;

		succeeded = ecs_renderer_try_setup_messenger(*out);
		if (!succeeded)
			goto cleanup;

		return true;

cleanup:
		ecs_renderer_teardown(*out);
		*out = NULL;

		return false;
	}

	free(*out);
	*out = NULL;

	return false;
}

void ecs_renderer_teardown(struct ecs_renderer *renderer) {
	ECS_ASSERT(renderer, "Given renderer pointer is null!");

	if (ENABLED_VALIDATION_LAYERS)
		vk_destroy_debug_utils_messenger(renderer->instance, renderer->debug_messenger, vk_allocator);

	if (renderer->instance)
		vkDestroyInstance(renderer->instance, vk_allocator);

	free(renderer);
}

void ecs_renderer_render_frame(struct ecs_renderer *renderer) {
	ECS_ASSERT(renderer, "Given renderer pointer is null!");
}

static b8 ecs_renderer_try_create_instance(struct ecs_renderer *renderer) {
	b8 have_required_layers = vk_instance_supports_layers(VALIDATION_LAYER_COUNT, VALIDATION_LAYERS);
	if (ENABLED_VALIDATION_LAYERS && !have_required_layers)
		return false;

	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "libecs",
		.applicationVersion = VK_MAKE_VERSION(ECS_VERSION_MAJOR, ECS_VERSION_MINOR, ECS_VERSION_PATCH),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};
	
	/* setting required extensions */
	u32 required_extension_count = 0;
	const char **required_extensions = malloc(sizeof(const char*));

#ifndef NDEBUG
	const char *debug_utils_extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	b8 have_debug_utils_extension = vk_instance_supports_extensions(1, &debug_utils_extension);

	if (!have_debug_utils_extension)
		return false;

	required_extension_count += 1;
	required_extensions = realloc(required_extensions, sizeof(const char*) * required_extension_count);
	required_extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

	u32 glfw_extension_count = 0;
	const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	b8 have_glfw_extensions = vk_instance_supports_extensions(glfw_extension_count, glfw_extensions);

	if (!have_glfw_extensions)
		return false;

	u32 copy_start = required_extension_count;
	required_extension_count += glfw_extension_count;
	required_extensions = realloc(required_extensions, sizeof(const char*) * required_extension_count);
	for (usize i = copy_start; i < required_extension_count; i++) {
		required_extensions[i] = glfw_extensions[i - copy_start];
	}

	/* constructing VkInstance */
	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application_info,
		.enabledExtensionCount = required_extension_count,
		.ppEnabledExtensionNames = required_extensions,
		.pNext = NULL,
	};

	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info;
	if (ENABLED_VALIDATION_LAYERS) {
		messenger_create_info = vk_get_debug_utils_messenger_create_info();
		instance_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
		instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
		instance_create_info.pNext = &messenger_create_info;
	}

	VkResult result = vkCreateInstance(&instance_create_info, vk_allocator, &renderer->instance);
	free(required_extensions);
	return result == VK_SUCCESS;
}

static b8 ecs_renderer_try_setup_messenger(struct ecs_renderer *renderer) {
	if (!ENABLED_VALIDATION_LAYERS)
		return true;

	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = vk_get_debug_utils_messenger_create_info();

	VkResult result = vk_create_debug_utils_messenger(renderer->instance, &messenger_create_info, vk_allocator, &renderer->debug_messenger);
	return result == VK_SUCCESS;
}

