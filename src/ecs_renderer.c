#include "ecs.h"
#include "ecs_version.h"
#include "ecs_renderer.h"

#include <string.h>
#include <cglm/cglm.h>

const VkAllocationCallbacks *vk_allocator = NULL;

const char *VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation",
};
const usize VALIDATION_LAYER_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(const char*);

const char *DEVICE_EXTENSIONS[] = {
	"VK_KHR_swapchain",
};
const usize DEVICE_EXTENSION_COUNT = sizeof(DEVICE_EXTENSIONS) / sizeof(const char*);

const struct ecs_vertex vertices[] = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
const usize vertex_count = sizeof(vertices) / sizeof(struct ecs_vertex);

const u32 indices[] = {
	0, 1, 2, 2, 3, 0
};
const usize index_count = sizeof(indices) / sizeof(u32);

#define MAX_FRAMES_IN_FLIGHT 2

VKAPI_ATTR VkBool32 VKAPI_CALL vk_layer_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data) {
	printf("%s\n", callback_data->pMessage);

	return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT vk_debug_utils_messenger_create_info() {
	VkDebugUtilsMessageSeverityFlagsEXT severity = 0;
	/* severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT; */
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

static const VkVertexInputBindingDescription ECS_VERTEX_BINDING_DESCRIPTION = {
	.binding = 0,
	.stride = sizeof(struct ecs_vertex),
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

static const VkVertexInputAttributeDescription ECS_VERTEX_ATTRIBUTE_DESCRIPTIONS[] = {
	{.binding = 0, .location = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(struct ecs_vertex, pos)},
	{.binding = 0, .location = 1, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(struct ecs_vertex, colour)},
};
const usize ECS_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT = sizeof(ECS_VERTEX_ATTRIBUTE_DESCRIPTIONS) / sizeof(VkVertexInputAttributeDescription);

struct queue_family_indices {
	u32 graphics_family;
	u32 present_family;
	b8 has_graphics_family : 1;
	b8 has_present_family : 1;
};

static b8 queue_family_indices_is_complete(struct queue_family_indices indices) {
	return indices.has_graphics_family && indices.has_present_family;
}

struct swapchain_support_details {
	VkSurfaceCapabilitiesKHR capabilities;
	usize format_count;
	VkSurfaceFormatKHR *formats;
	usize present_mode_count;
	VkPresentModeKHR *present_modes;
};

struct ecs_renderer {
	u32 width, height;
	GLFWwindow *window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physical_device;
	VkDevice device;

	VkQueue graphics_queue;
	VkQueue present_queue;

	VkSwapchainKHR swapchain;
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_extent;
	u32 swapchain_image_count;
	VkImage *swapchain_images;
	VkImageView *swapchain_image_views;
	VkFramebuffer *swapchain_framebuffers;
	b8 framebuffers_were_resized;

	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;

	VkCommandPool command_pool;
	VkCommandBuffer *command_buffers;

	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	VkFence *images_in_flight;
	usize current_frame;
};


static b8 vk_instance_supports_extensions(usize count, const char **extensions) {
	u32 instance_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);

	VkExtensionProperties *instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
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

static b8 vk_instance_supports_layers(usize count, const char **layers) {
	u32 instance_layer_count = 0;
	vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);

	VkLayerProperties *instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
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

static b8 vk_physical_device_supports_extensions(VkPhysicalDevice device, usize count, const char **extensions) {
	u32 device_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device, NULL, &device_extension_count, NULL);

	VkExtensionProperties *device_extensions = malloc(sizeof(VkExtensionProperties) * device_extension_count);
	vkEnumerateDeviceExtensionProperties(device, NULL, &device_extension_count, device_extensions);

	for (usize i = 0; i < count; i++) {
		const char *required_extension = extensions[i];

		b8 extension_found = false;
		for (usize j = 0; j < device_extension_count; j++) {
			VkExtensionProperties extension = device_extensions[j];

			if (strcmp(required_extension, extension.extensionName) == 0) {
				extension_found = true;
				break;
			}
		}

		if (!extension_found) {
			free(device_extensions);

			return false;	
		}
	}

	free(device_extensions);

	return true;
}

static struct queue_family_indices find_queue_families(struct ecs_renderer *renderer, VkPhysicalDevice device) {
	VkSurfaceKHR renderer_surface = renderer->surface;
	struct queue_family_indices indices = {0};

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

	VkQueueFamilyProperties *queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

	for (usize i = 0; i < queue_family_count; i++) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
			indices.has_graphics_family = true;
		}

		VkBool32 present_queue_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, renderer_surface, &present_queue_support);

		if (present_queue_support) {
			indices.present_family = i;
			indices.has_present_family = true;
		}

		if (queue_family_indices_is_complete(indices))
			break;
	}

	free(queue_families);

	return indices;
}

static struct swapchain_support_details query_swapchain_support(struct ecs_renderer *renderer, VkPhysicalDevice device) {
	VkSurfaceKHR renderer_surface = renderer->surface;
	struct swapchain_support_details details = {0};	

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, renderer_surface, &details.capabilities);

	u32 surface_format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderer_surface, &surface_format_count, NULL);

	if (surface_format_count > 0) {
		details.format_count = surface_format_count;
		details.formats = malloc(sizeof(VkSurfaceFormatKHR) * surface_format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderer_surface, &surface_format_count, details.formats);
	}

	u32 surface_present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderer_surface, &surface_present_mode_count, NULL);

	if (surface_present_mode_count > 0) {
		details.present_mode_count = surface_present_mode_count;
		details.present_modes = malloc(sizeof(VkPresentModeKHR) * surface_present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderer_surface, &surface_present_mode_count, details.present_modes);
	}

	return details;
}

// NOTE(mikolaj): add some form of scoring to favour discrete GPUs over iGPUs?
static b8 physical_device_is_suitable(struct ecs_renderer *renderer, VkPhysicalDevice device) {
	struct queue_family_indices indices = find_queue_families(renderer, device);

	b8 device_extensions_supported = vk_physical_device_supports_extensions(device, DEVICE_EXTENSION_COUNT, DEVICE_EXTENSIONS);

	b8 swapchain_adequate = false;
	if (device_extensions_supported) {
		struct swapchain_support_details details = query_swapchain_support(renderer, device);
		swapchain_adequate = details.format_count > 0 && details.present_mode_count > 0;
	}

	return queue_family_indices_is_complete(indices) && device_extensions_supported && swapchain_adequate;
}

static usize read_sprv_shader(const char *path, u32 **out) {
	FILE *shader_file = fopen(path, "rb");

	int err = fseek(shader_file, 0, SEEK_END);
	int size = ftell(shader_file);
	rewind(shader_file);

	*out = malloc(size);
	if (*out) {
		usize read = fread(*out, sizeof(char), size / sizeof(char), shader_file);
		ECS_ASSERT(read == (usize)size, "Number of bytes read from shader does not match file size!");

		fclose(shader_file);
		return read;
	}

	return -1;
}

static b8 try_create_shader_module(VkDevice device, usize count, u32 *shader_code, VkShaderModule *out) {
	VkShaderModuleCreateInfo module_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = count,
		.pCode = shader_code,
	};

	VkResult result = vkCreateShaderModule(device, &module_create_info, vk_allocator, out);
	return result == VK_SUCCESS;
}

static b8 try_find_memory_type(VkPhysicalDevice physical_device, u32 memory_type_mask, VkMemoryPropertyFlags flags, u32 *out) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
		b8 property_flags_match = (memory_properties.memoryTypes[i].propertyFlags & flags) == flags;
		if (memory_type_mask & (1 << i) && property_flags_match) {
			*out = i;
			return true;
		}
	}

	return false;
}

static b8 try_create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory) {
	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if (vkCreateBuffer(device, &buffer_create_info, vk_allocator, buffer) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memory_requirements);

	u32 memory_type_index;
	if (!try_find_memory_type(physical_device, memory_requirements.memoryTypeBits, properties, &memory_type_index)) {
		return false;
	}

	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type_index,
	};

	if (vkAllocateMemory(device, &memory_allocate_info, vk_allocator, buffer_memory) != VK_SUCCESS) {
		return false;
	}

	vkBindBufferMemory(device, *buffer, *buffer_memory, 0);

	return true;
}

static b8 try_copy_buffer(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
	VkCommandBufferAllocateInfo copy_command_buffer_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = command_pool,
		.commandBufferCount = 1,
	};

	VkCommandBuffer copy_command_buffer;
	vkAllocateCommandBuffers(device, &copy_command_buffer_allocate_info, &copy_command_buffer);

	VkCommandBufferBeginInfo copy_command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(copy_command_buffer, &copy_command_buffer_begin_info);

	VkBufferCopy copy_region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(copy_command_buffer, src, dst, 1, &copy_region);

	vkEndCommandBuffer(copy_command_buffer);

	VkSubmitInfo copy_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &copy_command_buffer,
	};

	// TODO(mikolaj): consider fences to allow multiple memory copies to 
	// occur in parallel?
	vkQueueSubmit(graphics_queue, 1, &copy_submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &copy_command_buffer);

	return true;
}

static b8 ecs_renderer_try_create_instance(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_setup_debug_utils_messenger(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_surface(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_select_physical_device(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_logical_device(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_swapchain(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_recreate_swapchain(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_image_views(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_render_pass(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_graphics_pipeline(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_framebuffers(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_command_pool(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_vertex_buffers(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_index_buffers(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_command_buffers(struct ecs_renderer *renderer);
static b8 ecs_renderer_try_create_semaphores(struct ecs_renderer *renderer);

static void ecs_renderer_cleanup_swapchain(struct ecs_renderer *renderer);

static void ecs_renderer_framebuffers_resized_callback(GLFWwindow *window, int width, int height) {
	struct ecs_renderer *renderer = glfwGetWindowUserPointer(window);
	renderer->framebuffers_were_resized = true;
}

b8 ecs_renderer_try_alloc(u32 width, u32 height, GLFWwindow *window, struct ecs_renderer **out) {
	ECS_ASSERT(window, "Given GLFW window pointer is null!");
	ECS_ASSERT(out, "Given out pointer is null!");
	ECS_ASSERT_NE(*out, "Given out pointer points to non-null value (would overwrite existing renderer)!");

	*out = malloc(sizeof(struct ecs_renderer));
	if (*out) {
		memset(*out, 0, sizeof(struct ecs_renderer));

		(*out)->width = width;
		(*out)->height = height;
		(*out)->window = window;

		glfwSetWindowUserPointer((*out)->window, *out);
		glfwSetFramebufferSizeCallback((*out)->window, ecs_renderer_framebuffers_resized_callback);

		b8 succeeded = false;
		
		succeeded = ecs_renderer_try_create_instance(*out);
		if (!succeeded) {
			printf("Could not create Vulkan instance!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_setup_debug_utils_messenger(*out);
		if (!succeeded) {
			printf("Could not set up validation layers or debug messenger!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_surface(*out);
		if (!succeeded) {
			printf("Could not create render surface!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_select_physical_device(*out);
		if (!succeeded) {
			printf("Could not select a suitable physical device!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_logical_device(*out);
		if (!succeeded) {
			printf("Could not create a logical device from the selected physical device!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_swapchain(*out);
		if (!succeeded) {
			printf("Could not create a swapchain for the logical device!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_image_views(*out);
		if (!succeeded) {
			printf("Could not create image views for each image in swapchain!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_render_pass(*out);
		if (!succeeded) {
			printf("Could not create render pass for graphics pipeline!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_graphics_pipeline(*out);
		if (!succeeded) {
			printf("Could not create graphics pipeline!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_framebuffers(*out);
		if (!succeeded) {
			printf("Could not create framebuffers for image views!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_command_pool(*out);
		if (!succeeded) {
			printf("Could not create command pool!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_vertex_buffers(*out);
		if (!succeeded) {
			printf("Could not create vertex buffers!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_index_buffers(*out);
		if (!succeeded) {
			printf("Could not create index buffers!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_command_buffers(*out);
		if (!succeeded) {
			printf("Could not create command buffers!\n");
			goto cleanup;
		}

		succeeded = ecs_renderer_try_create_semaphores(*out);
		if (!succeeded) {
			printf("Could not create synchronisation semaphores!\n");
			goto cleanup;
		}

		return true;

cleanup:
		ecs_renderer_free(*out);
		*out = NULL;

		return false;
	}

	free(*out);
	*out = NULL;

	return false;
}

void ecs_renderer_free(struct ecs_renderer *renderer) {
	ECS_ASSERT(renderer, "Given renderer pointer is null!");

	vkDeviceWaitIdle(renderer->device);

	ecs_renderer_cleanup_swapchain(renderer);

	vkDestroyBuffer(renderer->device, renderer->index_buffer, vk_allocator);
	vkFreeMemory(renderer->device, renderer->index_buffer_memory, vk_allocator);

	vkDestroyBuffer(renderer->device, renderer->vertex_buffer, vk_allocator);
	vkFreeMemory(renderer->device, renderer->vertex_buffer_memory, vk_allocator);

	for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(renderer->device, renderer->render_finished_semaphores[i], vk_allocator);
		vkDestroySemaphore(renderer->device, renderer->image_available_semaphores[i], vk_allocator);
		vkDestroyFence(renderer->device, renderer->in_flight_fences[i], vk_allocator);
	}

	vkDestroyCommandPool(renderer->device, renderer->command_pool, vk_allocator);

	vkDestroyDevice(renderer->device, vk_allocator);

#ifndef NDEBUG
	vk_destroy_debug_utils_messenger(renderer->instance, renderer->debug_messenger, vk_allocator);
#endif

	vkDestroySurfaceKHR(renderer->instance, renderer->surface, vk_allocator);
	vkDestroyInstance(renderer->instance, vk_allocator);

	free(renderer);
}

void ecs_renderer_render_frame(struct ecs_renderer *renderer) {
	ECS_ASSERT(renderer, "Given renderer pointer is null!");

	vkWaitForFences(renderer->device, 1, &renderer->in_flight_fences[renderer->current_frame], VK_TRUE, UINT64_MAX);

	u32 image_index;
	VkResult acquire_result = vkAcquireNextImageKHR(renderer->device, renderer->swapchain, UINT64_MAX, renderer->image_available_semaphores[renderer->current_frame], VK_NULL_HANDLE, &image_index);

	if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
		if (!ecs_renderer_try_recreate_swapchain(renderer))
			printf("Failed to recreate swapchain!\n");
		return;
	} else if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
		printf("Could not acquire swapchain image!\n");
	}

	if (renderer->images_in_flight[image_index] != VK_NULL_HANDLE) {
		vkWaitForFences(renderer->device, 1, &renderer->images_in_flight[image_index], VK_TRUE, UINT64_MAX);
	}
	renderer->images_in_flight[image_index] = renderer->in_flight_fences[renderer->current_frame];

	const VkSemaphore wait_semaphores[] = {
		renderer->image_available_semaphores[renderer->current_frame],
	};

	const VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	};

	const VkSemaphore signal_semaphores[] = {
		renderer->render_finished_semaphores[renderer->current_frame],
	};

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &renderer->command_buffers[image_index],
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores,
	};

	vkResetFences(renderer->device, 1, &renderer->in_flight_fences[renderer->current_frame]);

	if (vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, renderer->in_flight_fences[renderer->current_frame]) != VK_SUCCESS)
		printf("Failed to submit work to the graphics queue!\n");

	const VkSwapchainKHR swapchains[] = {
		renderer->swapchain,
	};

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &image_index,
	};

	VkResult present_result = vkQueuePresentKHR(renderer->present_queue, &present_info);

	if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR || renderer->framebuffers_were_resized) {
		renderer->framebuffers_were_resized = false;
		if (!ecs_renderer_try_recreate_swapchain(renderer))
			printf("Failed to recreate swapchain!\n");
	} else if (present_result != VK_SUCCESS) {
		printf("Failed to present swapchain image!\n");
	}

	renderer->current_frame = (renderer->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

static b8 ecs_renderer_try_create_instance(struct ecs_renderer *renderer) {
#ifndef NDEBUG
	b8 have_required_layers = vk_instance_supports_layers(VALIDATION_LAYER_COUNT, VALIDATION_LAYERS);
	if (!have_required_layers)
		return false;
#endif

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
	required_extensions = realloc(required_extensions, sizeof(const char*));
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

#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = vk_debug_utils_messenger_create_info();
	instance_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
	instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
	instance_create_info.pNext = &messenger_create_info;
#endif

	VkResult result = vkCreateInstance(&instance_create_info, vk_allocator, &renderer->instance);

	free(required_extensions);

	return result == VK_SUCCESS;
}

static b8 ecs_renderer_try_setup_debug_utils_messenger(struct ecs_renderer *renderer) {
#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT messenger_create_info = vk_debug_utils_messenger_create_info();

	VkResult result = vk_create_debug_utils_messenger(renderer->instance, &messenger_create_info, vk_allocator, &renderer->debug_messenger);
	return result == VK_SUCCESS;
#else
	return true;
#endif
}

static b8 ecs_renderer_try_create_surface(struct ecs_renderer *renderer) {
	VkResult result = glfwCreateWindowSurface(renderer->instance, renderer->window, vk_allocator, &renderer->surface);

	return result == VK_SUCCESS;
}

static b8 ecs_renderer_try_select_physical_device(struct ecs_renderer *renderer) {
	renderer->physical_device = VK_NULL_HANDLE;

	u32 physical_device_count = 0;
	vkEnumeratePhysicalDevices(renderer->instance, &physical_device_count, NULL);

	if (physical_device_count == 0)
		return false;

	VkPhysicalDevice *physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(renderer->instance, &physical_device_count, physical_devices);

	for (usize i = 0; i < physical_device_count; i++) {
		VkPhysicalDevice physical_device = physical_devices[i];
		if (physical_device_is_suitable(renderer, physical_device)) {
			renderer->physical_device = physical_device;
			break;
		}
	}

	free(physical_devices);

	return renderer->physical_device != VK_NULL_HANDLE;
}

static b8 ecs_renderer_try_create_logical_device(struct ecs_renderer *renderer) {
	struct queue_family_indices indices = find_queue_families(renderer, renderer->physical_device);

	const float queue_priorities[] = { 1.0f };
	VkDeviceQueueCreateInfo graphics_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = indices.graphics_family,
		.queueCount = 1,
		.pQueuePriorities = queue_priorities,
	};

	VkDeviceQueueCreateInfo present_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = indices.present_family,
		.queueCount = 1,
		.pQueuePriorities = queue_priorities,
	};

	const VkDeviceQueueCreateInfo queue_create_infos[] = {
		graphics_queue_create_info,
		present_queue_create_info,
	};

	VkPhysicalDeviceFeatures physical_device_features = {0};

	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 2,
		.pQueueCreateInfos = queue_create_infos,
		.pEnabledFeatures = &physical_device_features,
		.enabledExtensionCount = DEVICE_EXTENSION_COUNT,
		.ppEnabledExtensionNames = DEVICE_EXTENSIONS,
	};

#ifndef NDEBUG
	device_create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
	device_create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
#endif

	VkResult result = vkCreateDevice(renderer->physical_device, &device_create_info, vk_allocator, &renderer->device);
	if (result != VK_SUCCESS)
		return false;

	vkGetDeviceQueue(renderer->device, indices.graphics_family, 0, &renderer->graphics_queue);
	vkGetDeviceQueue(renderer->device, indices.present_family, 0, &renderer->present_queue);

	return true;
}

static b8 ecs_renderer_try_create_swapchain(struct ecs_renderer *renderer) {
	struct swapchain_support_details details = query_swapchain_support(renderer, renderer->physical_device);

	/* selecting a surface format */
	VkSurfaceFormatKHR surface_format = details.formats[0];
	for (usize i = 0; i < details.format_count; i++) {
		VkSurfaceFormatKHR available_format = details.formats[i];

		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surface_format = available_format;
			break;
		}
	}

	/* selecting a surface present mode */
	VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (usize i = 0; i < details.present_mode_count; i++) {
		VkPresentModeKHR available_present_mode = details.present_modes[i];

		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			surface_present_mode = available_present_mode;
			break;
		}
	}

	/* selecting a surface extent */
	VkExtent2D surface_extent = details.capabilities.currentExtent;
	if (details.capabilities.currentExtent.width == UINT32_MAX) {
		int width, height;
		glfwGetFramebufferSize(renderer->window, &width, &height);

		u32 actual_width = (u32)width;
		u32 actual_height = (u32)height;
		u32 min_width = details.capabilities.minImageExtent.width;
		u32 min_height = details.capabilities.minImageExtent.height;
		u32 max_width = details.capabilities.maxImageExtent.width;
		u32 max_height = details.capabilities.maxImageExtent.height;

		/* surface_extent = max(details.capabilities.minImageExtent, min(details.capabilities.maxImageExtent, actual_extent)); */

		u32 clamped_width = max_width < actual_width ? max_width : actual_width;
		u32 clamped_height = max_height < actual_height ? max_height : actual_height;

		surface_extent.width = min_width > clamped_width ? min_width : clamped_width;
		surface_extent.height = min_height > clamped_height ? min_height : clamped_height;
	}

	/* + 1 to provide a small buffer of images to display, instead of having to block on the driver before we can aquire a new image */ 
	u32 swapchain_image_count = details.capabilities.minImageCount + 1;

	/* we clamp to maxImageCount, except where said value is 0 (meaning no maximum count) */
	if (details.capabilities.maxImageCount > 0 && swapchain_image_count > details.capabilities.maxImageCount) {
		swapchain_image_count = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = renderer->surface,
		.minImageCount = swapchain_image_count,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = surface_extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // TODO(mikolaj): switch to VK_IMAGE_USAGE_TRANSFER_DST_BIT to allow post-processing
		.preTransform = details.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surface_present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE, // TODO(mikolaj): revisit in a later chapter
	};

	struct queue_family_indices indices = find_queue_families(renderer, renderer->physical_device);
	const u32 family_indices[] = {
		indices.graphics_family,
		indices.present_family
	};

	if (indices.graphics_family != indices.present_family) {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = sizeof(family_indices) / sizeof(u32);
		swapchain_create_info.pQueueFamilyIndices = family_indices;
	} else {
		/* if our queues are the same, we can use the more performant EXCLUSIVE sharing mode */
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkResult result = vkCreateSwapchainKHR(renderer->device, &swapchain_create_info, vk_allocator, &renderer->swapchain);

	if (result != VK_SUCCESS)
		return false;

	vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain, &renderer->swapchain_image_count, NULL);
	if (renderer->swapchain_images) {
		renderer->swapchain_images = realloc(renderer->swapchain_images, sizeof(VkImage) * renderer->swapchain_image_count);
	} else {
		renderer->swapchain_images = malloc(sizeof(VkImage) * renderer->swapchain_image_count);
	}
	vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain, &renderer->swapchain_image_count, renderer->swapchain_images);

	renderer->swapchain_image_format = surface_format.format;
	renderer->swapchain_extent = surface_extent;

	return true;
}

static b8 ecs_renderer_try_recreate_swapchain(struct ecs_renderer *renderer) {
	int width = 0, height = 0;
	do {
		glfwGetFramebufferSize(renderer->window, &width, &height);
		glfwWaitEvents();
	} while (width == 0 || height == 0);

	vkDeviceWaitIdle(renderer->device);

	ecs_renderer_cleanup_swapchain(renderer);

	b8 succeeded = false;
	succeeded = ecs_renderer_try_create_swapchain(renderer);
	if (!succeeded)
		return false;

	succeeded = ecs_renderer_try_create_image_views(renderer);
	if (!succeeded)
		return false;

	succeeded = ecs_renderer_try_create_render_pass(renderer);
	if (!succeeded)
		return false;

	succeeded = ecs_renderer_try_create_graphics_pipeline(renderer);
	if (!succeeded)
		return false;

	succeeded = ecs_renderer_try_create_framebuffers(renderer);
	if (!succeeded)
		return false;

	succeeded = ecs_renderer_try_create_command_buffers(renderer);

	return succeeded;
}

static void ecs_renderer_cleanup_swapchain(struct ecs_renderer *renderer) {
	for (usize i = 0; i < renderer->swapchain_image_count; i++) {
		vkDestroyFramebuffer(renderer->device, renderer->swapchain_framebuffers[i], vk_allocator);
	}

	vkFreeCommandBuffers(renderer->device, renderer->command_pool, renderer->swapchain_image_count, renderer->command_buffers);

	vkDestroyPipeline(renderer->device, renderer->pipeline, vk_allocator);
	vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, vk_allocator);
	vkDestroyRenderPass(renderer->device, renderer->render_pass, vk_allocator);

	for (usize i = 0; i < renderer->swapchain_image_count; i++) {
		vkDestroyImageView(renderer->device, renderer->swapchain_image_views[i], vk_allocator);
	}

	vkDestroySwapchainKHR(renderer->device, renderer->swapchain, vk_allocator);
}

static b8 ecs_renderer_try_create_image_views(struct ecs_renderer *renderer) {
	if (renderer->swapchain_image_views) {
		renderer->swapchain_image_views = realloc(renderer->swapchain_image_views, sizeof(VkImageView) * renderer->swapchain_image_count);
	} else {
		renderer->swapchain_image_views = malloc(sizeof(VkImageView) * renderer->swapchain_image_count);
	}

	for (usize i = 0; i < renderer->swapchain_image_count; i++) {
		VkComponentMapping component_mapping = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		};

		VkImageSubresourceRange subresource_range = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};

		VkImageViewCreateInfo view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = renderer->swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = renderer->swapchain_image_format,
			.components = component_mapping,
			.subresourceRange = subresource_range,
		};

		VkResult result = vkCreateImageView(renderer->device, &view_create_info, vk_allocator, &renderer->swapchain_image_views[i]);
		
		if (result != VK_SUCCESS)
			return false;
	}

	return true;
}

static b8 ecs_renderer_try_create_render_pass(struct ecs_renderer *renderer) {
	VkAttachmentDescription colour_attachment_description = {
		.format = renderer->swapchain_image_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference colour_attachment_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass_description = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour_attachment_reference,
	};

	VkSubpassDependency subpass_dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo render_pass_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colour_attachment_description,
		.subpassCount = 1,
		.pSubpasses = &subpass_description,
		.dependencyCount = 1,
		.pDependencies = &subpass_dependency,
	};

	VkResult result = vkCreateRenderPass(renderer->device, &render_pass_create_info, vk_allocator, &renderer->render_pass);
	return result == VK_SUCCESS;
}

static b8 ecs_renderer_try_create_graphics_pipeline(struct ecs_renderer *renderer) {
	u32 *vert_shader = NULL;
	usize vert_shader_size = read_sprv_shader("out/base.vert.spv", &vert_shader);
	ECS_ASSERT(vert_shader, "Could not load vertex shader!");

	u32 *frag_shader = NULL;
	usize frag_shader_size = read_sprv_shader("out/base.frag.spv", &frag_shader);
	ECS_ASSERT(frag_shader, "Could not load fragment shader!");

	/* shader modules: vertex, fragment (unused: tesselation, geometry) */

	VkShaderModule vert_shader_module;
	b8 created_vert_shader = try_create_shader_module(renderer->device, vert_shader_size, vert_shader, &vert_shader_module);
	ECS_ASSERT(created_vert_shader, "Could not create vertex shader module!");

	VkShaderModule frag_shader_module;
	b8 created_frag_shader = try_create_shader_module(renderer->device, frag_shader_size, frag_shader, &frag_shader_module);
	ECS_ASSERT(created_frag_shader, "Could not create fragment shader module!");

	VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_shader_module,
		.pName = "main",
	};

	VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_shader_module,
		.pName = "main",
	};

	const VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_shader_stage_create_info,
		frag_shader_stage_create_info,
	};

	/* fixed pipeline functions: input assembly, rasterization, and colour blending */
	
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &ECS_VERTEX_BINDING_DESCRIPTION,
		.vertexAttributeDescriptionCount = ECS_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT,
		.pVertexAttributeDescriptions = ECS_VERTEX_ATTRIBUTE_DESCRIPTIONS,
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkViewport framebuffer_viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)renderer->swapchain_extent.width,
		.height = (float)renderer->swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	/* aka scissors, only regions of the viewport within this mask will be rendered  */
	VkRect2D framebuffer_viewport_mask = {
		.offset = {0, 0},
		.extent = renderer->swapchain_extent,
	};

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &framebuffer_viewport,
		.scissorCount = 1,
		.pScissors = &framebuffer_viewport_mask,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
	};

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	// TODO(mikolaj): Depth buffering and stencil testing. For a later chapter
	
	VkPipelineColorBlendAttachmentState colour_blend_attachment_state = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT  | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
	};

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colour_blend_attachment_state,
		.blendConstants[0] = 0.0f,
		.blendConstants[1] = 0.0f,
		.blendConstants[2] = 0.0f,
		.blendConstants[3] = 0.0f,
	};

	// TODO(mikolaj): dynamic states?

	VkPipelineLayoutCreateInfo layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
	};

	VkResult result;
	result = vkCreatePipelineLayout(renderer->device, &layout_create_info, vk_allocator, &renderer->pipeline_layout);

	if (result != VK_SUCCESS)
		goto cleanup;

	VkGraphicsPipelineCreateInfo pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo),
		.pStages = shader_stages,
		.pVertexInputState = &vertex_input_state_create_info,
		.pInputAssemblyState = &input_assembly_state_create_info,
		.pViewportState = &viewport_state_create_info,
		.pRasterizationState = &rasterizer_state_create_info,
		.pMultisampleState = &multisample_state_create_info,
		.pDepthStencilState = NULL,
		.pColorBlendState = &colour_blend_state_create_info,
		.pDynamicState = NULL,
		.layout = renderer->pipeline_layout,
		.renderPass = renderer->render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
	};

	result = vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipeline_create_info, vk_allocator, &renderer->pipeline);

cleanup:
	vkDestroyShaderModule(renderer->device, frag_shader_module, vk_allocator);
	free(frag_shader);

	vkDestroyShaderModule(renderer->device, vert_shader_module, vk_allocator);
	free(vert_shader);

	return result == VK_SUCCESS;
}

static b8 ecs_renderer_try_create_framebuffers(struct ecs_renderer *renderer) {
	if (renderer->swapchain_framebuffers) {
		renderer->swapchain_framebuffers = realloc(renderer->swapchain_framebuffers, sizeof(VkFramebuffer) * renderer->swapchain_image_count);
	} else {
		renderer->swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * renderer->swapchain_image_count);
	}

	for (usize i = 0; i < renderer->swapchain_image_count; i++) {
		VkImageView attachments[] = {
			renderer->swapchain_image_views[i],
		};

		VkFramebufferCreateInfo framebuffer_create_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderer->render_pass,
			.attachmentCount = sizeof(attachments) / sizeof(VkImageView),
			.pAttachments = attachments,
			.width = renderer->swapchain_extent.width,
			.height = renderer->swapchain_extent.height,
			.layers = 1,
		};

		VkResult result = vkCreateFramebuffer(renderer->device, &framebuffer_create_info, vk_allocator, &renderer->swapchain_framebuffers[i]);
		if (result != VK_SUCCESS)
			return false;
	}

	return true;
}

static b8 ecs_renderer_try_create_command_pool(struct ecs_renderer *renderer) {
	struct queue_family_indices indices = find_queue_families(renderer, renderer->physical_device);

	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = indices.graphics_family,
		.flags = 0,
	};

	VkResult result = vkCreateCommandPool(renderer->device, &command_pool_create_info, vk_allocator, &renderer->command_pool);

	return result == VK_SUCCESS;
}

static b8 ecs_renderer_try_create_vertex_buffers(struct ecs_renderer *renderer) {
	VkDeviceSize vertex_buffer_size = sizeof(struct ecs_vertex) * vertex_count;

	VkBufferUsageFlags staging_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags staging_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	if (!try_create_buffer(renderer->device, renderer->physical_device, vertex_buffer_size, staging_usage_flags, staging_property_flags, &staging_buffer, &staging_buffer_memory)) {
		return false;
	}

	void* data;
	vkMapMemory(renderer->device, staging_buffer_memory, 0, vertex_buffer_size, 0, &data);
	memcpy(data, vertices, vertex_buffer_size);
	vkUnmapMemory(renderer->device, staging_buffer_memory);


	VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	VkMemoryPropertyFlags property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (!try_create_buffer(renderer->device, renderer->physical_device, vertex_buffer_size, usage_flags, property_flags, &renderer->vertex_buffer, &renderer->vertex_buffer_memory)) {
		return false;
	}

	if(!try_copy_buffer(renderer->device, renderer->command_pool, renderer->graphics_queue, staging_buffer, renderer->vertex_buffer, vertex_buffer_size)) {
		return false;
	}

	vkDestroyBuffer(renderer->device, staging_buffer, vk_allocator);
	vkFreeMemory(renderer->device, staging_buffer_memory, vk_allocator);

	return true;
}

static b8 ecs_renderer_try_create_index_buffers(struct ecs_renderer *renderer) {
	VkDeviceSize index_buffer_size = sizeof(u32) * index_count;

	VkBufferUsageFlags staging_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags staging_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	if (!try_create_buffer(renderer->device, renderer->physical_device, index_buffer_size, staging_usage_flags, staging_property_flags, &staging_buffer, &staging_buffer_memory)) {
		return false;
	}

	void* data;
	vkMapMemory(renderer->device, staging_buffer_memory, 0, index_buffer_size, 0, &data);
	memcpy(data, indices, index_buffer_size);
	vkUnmapMemory(renderer->device, staging_buffer_memory);

	VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	VkMemoryPropertyFlags property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (!try_create_buffer(renderer->device, renderer->physical_device, index_buffer_size, usage_flags, property_flags, &renderer->index_buffer, &renderer->index_buffer_memory)) {
		return false;
	}

	if (!try_copy_buffer(renderer->device, renderer->command_pool, renderer->graphics_queue, staging_buffer, renderer->index_buffer, index_buffer_size)) {
		return false;
	}

	vkDestroyBuffer(renderer->device, staging_buffer, vk_allocator);
	vkFreeMemory(renderer->device, staging_buffer_memory, vk_allocator);

	return true;
}

static b8 ecs_renderer_try_create_command_buffers(struct ecs_renderer *renderer) {
	if (renderer->command_buffers) {
		renderer->command_buffers = realloc(renderer->command_buffers, sizeof(VkCommandBuffer) * renderer->swapchain_image_count);
	} else {
		renderer->command_buffers = malloc(sizeof(VkCommandBuffer) * renderer->swapchain_image_count);
	}

	VkCommandBufferAllocateInfo command_buffer_allocation_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = renderer->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (u32)renderer->swapchain_image_count,
	};

	VkResult result = vkAllocateCommandBuffers(renderer->device, &command_buffer_allocation_info, renderer->command_buffers);
	if (result != VK_SUCCESS)
		return false;

	for (usize i = 0; i < renderer->swapchain_image_count; i++) {
		VkCommandBufferBeginInfo command_buffer_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = NULL,
		};

		if (vkBeginCommandBuffer(renderer->command_buffers[i], &command_buffer_begin_info) != VK_SUCCESS)
			return false;

		VkClearValue colour_clear = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo render_pass_begin_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderer->render_pass,
			.framebuffer = renderer->swapchain_framebuffers[i],
			.renderArea.offset = {0, 0},
			.renderArea.extent = renderer->swapchain_extent,
			.clearValueCount = 1,
			.pClearValues = &colour_clear,
		};

		vkCmdBeginRenderPass(renderer->command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(renderer->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);

		VkBuffer vertex_buffers[] = { renderer->vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(renderer->command_buffers[i], 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(renderer->command_buffers[i], renderer->index_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(renderer->command_buffers[i], index_count, 1, 0, 0, 0);

		vkCmdEndRenderPass(renderer->command_buffers[i]);

		if (vkEndCommandBuffer(renderer->command_buffers[i]) != VK_SUCCESS)
			return false;
	}

	return true;
}

static b8 ecs_renderer_try_create_semaphores(struct ecs_renderer *renderer) {
	if (renderer->images_in_flight) {
		renderer->images_in_flight = realloc(renderer->images_in_flight, sizeof(VkFence) * renderer->swapchain_image_count);
	} else {
		renderer->images_in_flight = malloc(sizeof(VkFence) * renderer->swapchain_image_count);
	}

	for (usize i = 0; i < renderer->swapchain_image_count; i++) {
		renderer->images_in_flight[i] = VK_NULL_HANDLE;
	}

	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	VkResult result;
	for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		result = vkCreateSemaphore(renderer->device, &semaphore_create_info, vk_allocator, &renderer->image_available_semaphores[i]);
		if (result != VK_SUCCESS)
			return false;

		result = vkCreateSemaphore(renderer->device, &semaphore_create_info, vk_allocator, &renderer->render_finished_semaphores[i]);
		if (result != VK_SUCCESS)
			return false;

		result = vkCreateFence(renderer->device, &fence_create_info, vk_allocator, &renderer->in_flight_fences[i]);
		if (result != VK_SUCCESS)
			return false;
	}

	return true;
}

