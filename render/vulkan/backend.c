/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/render_lib
 *
 * The dependent library source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <foundation/foundation.h>
#include <window/window.h>
#include <render/render.h>
#include <render/internal.h>

#include <render/vulkan/backend.h>

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

#if FOUNDATION_PLATFORM_WINDOWS
#include <foundation/windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#if FOUNDATION_PLATFORM_LINUX
#include <vulkan/vulkan_xcb.h>
#endif

typedef struct render_backend_vulkan_t {
	RENDER_DECLARE_BACKEND;

	VkInstance instance;
	
	VkPhysicalDevice* adapter_available;
	uint adapter_count;
	VkPhysicalDevice adapter;
	VkPhysicalDeviceProperties adapter_properties;
	VkPhysicalDeviceFeatures adapter_features;

	VkDevice device;
	VkSurfaceKHR surface;
} render_backend_vulkan_t;

static bool
rb_vulkan_construct(render_backend_t* backend) {
	render_backend_vulkan_t* backend_vulkan = (render_backend_vulkan_t*)backend;
	if (backend_vulkan->instance)
		return true;

	const application_t* app = environment_application();

	VkApplicationInfo vk_app_info = {0};
	vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_app_info.pApplicationName = app->name.str;
	vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_app_info.pEngineName = "Neoengine";
	vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo vk_create_info = {0};
	vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vk_create_info.pApplicationInfo = &vk_app_info;

	// Instance extensions required
	uint extension_count = 0;
	bool found_surface_ext = false;
	bool found_platform_surface_ext = false;

#if FOUNDATION_PLATFORM_WINDOWS
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif FOUNDATION_PLATFORM_LINUX
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif

#define MAX_EXTENSION_COUNT 16
	const char* required_extension[MAX_EXTENSION_COUNT];
	uint required_extension_count = 0;

	VkResult result = vkEnumerateInstanceExtensionProperties(0, &extension_count, 0);
	if (extension_count > 0) {
		VkExtensionProperties* instance_extension =
		    memory_allocate(HASH_RENDER, sizeof(VkExtensionProperties) * extension_count, 0,
		                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		result = vkEnumerateInstanceExtensionProperties(0, &extension_count, instance_extension);
		for (uint32_t iext = 0;
		     (result == VK_SUCCESS) && (iext < extension_count) && (required_extension_count < MAX_EXTENSION_COUNT);
		     ++iext) {
			string_const_t extension_name = string_const(instance_extension[iext].extensionName,
			                                             string_length(instance_extension[iext].extensionName));
			if (string_equal(STRING_CONST(VK_KHR_SURFACE_EXTENSION_NAME), STRING_ARGS(extension_name))) {
				required_extension[required_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
				found_surface_ext = true;
			}
			if (string_equal(STRING_CONST(PLATFORM_SURFACE_EXTENSION_NAME), STRING_ARGS(extension_name))) {
				required_extension[required_extension_count++] = PLATFORM_SURFACE_EXTENSION_NAME;
				found_platform_surface_ext = 1;
			}
		}
		memory_deallocate(instance_extension);
	}

	if (!found_surface_ext) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan instance, missing extension %s"),
		           VK_KHR_SURFACE_EXTENSION_NAME);
		return false;
	}
	if (!found_platform_surface_ext) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan instance, missing extension %s"),
		           PLATFORM_SURFACE_EXTENSION_NAME);
		return false;
	}

	vk_create_info.enabledExtensionCount = required_extension_count;
	vk_create_info.ppEnabledExtensionNames = required_extension;
	vk_create_info.enabledLayerCount = 0;

	result = vkCreateInstance(&vk_create_info, nullptr, &backend_vulkan->instance);
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Failed to create Vulkan instance (%d)"),
		           (int)result);
		return false;
	}

	log_debug(HASH_RENDER, STRING_CONST("Constructed Vulkan render backend"));
	return true;
}

static void
rb_vulkan_destruct(render_backend_t* backend) {
	render_backend_vulkan_t* backend_vulkan = (render_backend_vulkan_t*)backend;

	if (backend_vulkan->adapter_available)
		memory_deallocate(backend_vulkan->adapter_available);
	backend_vulkan->adapter_available = 0;
	backend_vulkan->adapter_count = 0;
	backend_vulkan->adapter = 0;

	log_debug(HASH_RENDER, STRING_CONST("Destructed Vulkan render backend"));
}

static size_t
rb_vulkan_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	render_backend_vulkan_t* backend_vulkan = (render_backend_vulkan_t*)backend;
	if (!backend_vulkan->instance) {
		if (!rb_vulkan_construct(backend))
			return 0;
	}

	if (backend_vulkan->adapter_available)
		memory_deallocate(backend_vulkan->adapter_available);

	backend_vulkan->adapter = 0;
	backend_vulkan->adapter_count = 0;
	backend_vulkan->adapter_available = 0;

	VkResult result = vkEnumeratePhysicalDevices(backend_vulkan->instance, &backend_vulkan->adapter_count, 0);
	if ((result != VK_SUCCESS) || (backend_vulkan->adapter_count == 0)) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to enumerate Vulkan physical devices (%d)"), (int)result);
		return 0;
	}

	backend_vulkan->adapter_available = memory_allocate(HASH_RENDER, sizeof(VkPhysicalDevice) * backend_vulkan->adapter_count, 0,
	                                         MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	result = vkEnumeratePhysicalDevices(backend_vulkan->instance, &backend_vulkan->adapter_count,
	                                    backend_vulkan->adapter_available);
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to enumerate Vulkan physical devices (%d)"), (int)result);
		return 0;
	}

	// Arrange adapters in suitable order
	uint priority_order[5] = {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
	                          VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, VK_PHYSICAL_DEVICE_TYPE_CPU,
	                          VK_PHYSICAL_DEVICE_TYPE_OTHER};
	uint iadapter = 0;
	for (uint itype = 0; (itype < 5) && (iadapter < capacity); ++itype) {
		VkPhysicalDeviceProperties properties;
		for (uint32_t idev = 0; (idev < backend_vulkan->adapter_count) && (iadapter < capacity); ++idev) {
			vkGetPhysicalDeviceProperties(backend_vulkan->adapter_available[idev], &properties);
			if (properties.deviceType == priority_order[itype])
				store[iadapter++] = idev;
		}
	}

	return iadapter;
}

static size_t
rb_vulkan_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store,
                          size_t capacity) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(adapter);
	if (capacity) {
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, COLORSPACE_LINEAR, 60};
		store[0] = mode;
	}
	return 1;
}

static bool
rb_vulkan_set_drawable(render_backend_t* backend, const render_drawable_t* drawable) {
	render_backend_vulkan_t* backend_vulkan = (render_backend_vulkan_t*)backend;

	if (!backend_vulkan->adapter_available)
		rb_vulkan_enumerate_adapters(backend, 0, 0);
	if ((drawable->adapter != WINDOW_ADAPTER_DEFAULT) && (drawable->adapter  >= backend_vulkan->adapter_count)) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Unable to set Vulkan drawable, bad adapter index"));
		return false;
	}

	uint adapter_index = (drawable->adapter != WINDOW_ADAPTER_DEFAULT) ? drawable->adapter : 0;
	backend_vulkan->adapter = backend_vulkan->adapter_available[adapter_index];

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(backend_vulkan->adapter, &properties);
	log_infof(HASH_RENDER, STRING_CONST("Using Vulkan GPU %d: %s (type %u)"), adapter_index,
	          properties.deviceName, properties.deviceType);

	// Device extensions required
	uint32_t extension_count = 0;
	bool found_swapchain_ext = false;
#define MAX_EXTENSION_COUNT 16
	const char* required_extension[MAX_EXTENSION_COUNT];
	uint required_extension_count = 0;

	VkResult result = vkEnumerateDeviceExtensionProperties(backend_vulkan->adapter, 0, &extension_count, 0);
	if (extension_count > 0) {
		VkExtensionProperties* device_extension =
		    memory_allocate(HASH_RENDER, sizeof(VkExtensionProperties) * extension_count, 0,
		                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		result = vkEnumerateDeviceExtensionProperties(backend_vulkan->adapter, 0, &extension_count, device_extension);
		for (uint32_t iext = 0;
		     (result == VK_SUCCESS) && (iext < extension_count) && (required_extension_count < MAX_EXTENSION_COUNT);
		     ++iext) {
			string_const_t extension_name =
			    string_const(device_extension[iext].extensionName, string_length(device_extension[iext].extensionName));
			if (string_equal(STRING_CONST(VK_KHR_SWAPCHAIN_EXTENSION_NAME), STRING_ARGS(extension_name))) {
				required_extension[required_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
				found_swapchain_ext = true;
			}
		}
		memory_deallocate(device_extension);
	}

	if (!found_swapchain_ext) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to set Vulkan drawable, missing device extension %s"),
		           VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		return false;
	}

	// Create surface
#if FOUNDATION_PLATFORM_WINDOWS
	VkWin32SurfaceCreateInfoKHR surface_info;
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.hinstance = drawable->connection;
	surface_info.hwnd = drawable->hwnd;

	result = vkCreateWin32SurfaceKHR(backend_vulkan->instance, &surface_info, NULL, &backend_vulkan->surface);
#elif FOUNDATION_PLATFORM_LINUX
	VkXcbSurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.connection = drawable->connection;
	createInfo.window = drawable->xcb_window;

	result = vkCreateXcbSurfaceKHR(demo->inst, &createInfo, NULL, &demo->surface);
#endif
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to set Vulkan drawable, unable to create surface: %d"),
		           (int)result);
		return false;
	}

	// Initialize swapchain
	vkGetPhysicalDeviceProperties(backend_vulkan->adapter, &backend_vulkan->adapter_properties);
	vkGetPhysicalDeviceFeatures(backend_vulkan->adapter, &backend_vulkan->adapter_features);

	vkGetPhysicalDeviceQueueFamilyProperties(backend_vulkan->adapter, &demo->queue_family_count, NULL);
	assert(demo->queue_family_count >= 1);

	demo->queue_props = (VkQueueFamilyProperties*)malloc(demo->queue_family_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_family_count, demo->queue_props);

	// Iterate over each queue to learn whether it supports presenting:
	VkBool32* supportsPresent = (VkBool32*)malloc(demo->queue_family_count * sizeof(VkBool32));
	for (uint32_t i = 0; i < demo->queue_family_count; i++) {
		demo->fpGetPhysicalDeviceSurfaceSupportKHR(demo->gpu, i, demo->surface, &supportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t presentQueueFamilyIndex = UINT32_MAX;
	for (uint32_t i = 0; i < demo->queue_family_count; i++) {
		if ((demo->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (graphicsQueueFamilyIndex == UINT32_MAX) {
				graphicsQueueFamilyIndex = i;
			}

			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueFamilyIndex = i;
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	if (presentQueueFamilyIndex == UINT32_MAX) {
		// If didn't find a queue that supports both graphics and present, then
		// find a separate present queue.
		for (uint32_t i = 0; i < demo->queue_family_count; ++i) {
			if (supportsPresent[i] == VK_TRUE) {
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	// Generate error if could not find both a graphics and a present queue
	if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
		ERR_EXIT("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
	}

	demo->graphics_queue_family_index = graphicsQueueFamilyIndex;
	demo->present_queue_family_index = presentQueueFamilyIndex;
	demo->separate_present_queue = (demo->graphics_queue_family_index != demo->present_queue_family_index);
	free(supportsPresent);

    float queue_priorities[1] = {0.0};
	VkDeviceQueueCreateInfo queues[2];
	queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queues[0].pNext = NULL;
	queues[0].queueFamilyIndex = demo->graphics_queue_family_index;
	queues[0].queueCount = 1;
	queues[0].pQueuePriorities = queue_priorities;
	queues[0].flags = 0;

	VkDeviceCreateInfo device_info = {
	    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext = 0,
	    .queueCreateInfoCount = 1,
	    .pQueueCreateInfos = queues,
	    .enabledLayerCount = 0,
	    .ppEnabledLayerNames = 0,
	    .enabledExtensionCount = required_extension,
	    .ppEnabledExtensionNames = required_extension_count,
	    .pEnabledFeatures = 0
	};
	result = vkCreateDevice(backend_vulkan->adapter, &device_info, 0, &backend_vulkan->device);


	return true;
}

static void
rb_vulkan_dispatch(render_backend_t* backend, render_target_t* target, render_context_t** contexts,
                   size_t contexts_count) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
	FOUNDATION_UNUSED(contexts);
	FOUNDATION_UNUSED(contexts_count);
}

static void
rb_vulkan_flip(render_backend_t* backend) {
	++backend->framecount;
}

static void*
rb_vulkan_allocate_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	return memory_allocate(HASH_RENDER, buffer->buffersize, 16, MEMORY_PERSISTENT);
}

static void
rb_vulkan_deallocate_buffer(render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(aux);
	if (sys)
		memory_deallocate(buffer->store);
}

static bool
rb_vulkan_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	return true;
}

static bool
rb_vulkan_upload_shader(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(shader);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	return true;
}

static bool
rb_vulkan_upload_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(program);
	return true;
}

static bool
rb_vulkan_upload_texture(render_backend_t* backend, render_texture_t* texture, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(texture);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	return true;
}

static void
rb_vulkan_deallocate_texture(render_backend_t* backend, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(texture);
}

static void
rb_vulkan_parameter_bind_texture(render_backend_t* backend, void* buffer, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(texture);
}

static void
rb_vulkan_parameter_bind_target(render_backend_t* backend, void* buffer, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(target);
}

static void
rb_vulkan_link_buffer(render_backend_t* backend, render_buffer_t* buffer, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(program);
}

static void
rb_vulkan_deallocate_shader(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(shader);
}

static void
rb_vulkan_deallocate_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(program);
}

static bool
rb_vulkan_allocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
	return true;
}

static bool
rb_vulkan_resize_target(render_backend_t* backend, render_target_t* target, unsigned int width, unsigned int height) {
	FOUNDATION_UNUSED(backend);
	if (target) {
		target->width = width;
		target->height = height;
	}
	return true;
}

static void
rb_vulkan_deallocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
}

static void
rb_vulkan_enable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static void
rb_vulkan_disable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static render_backend_vtable_t render_backend_vtable_null = {.construct = rb_vulkan_construct,
                                                             .destruct = rb_vulkan_destruct,
                                                             .enumerate_adapters = rb_vulkan_enumerate_adapters,
                                                             .enumerate_modes = rb_vulkan_enumerate_modes,
                                                             .set_drawable = rb_vulkan_set_drawable,
                                                             .enable_thread = rb_vulkan_enable_thread,
                                                             .disable_thread = rb_vulkan_disable_thread,
                                                             .dispatch = rb_vulkan_dispatch,
                                                             .flip = rb_vulkan_flip,
                                                             .allocate_buffer = rb_vulkan_allocate_buffer,
                                                             .upload_buffer = rb_vulkan_upload_buffer,
                                                             .upload_shader = rb_vulkan_upload_shader,
                                                             .upload_program = rb_vulkan_upload_program,
                                                             .upload_texture = rb_vulkan_upload_texture,
                                                             .parameter_bind_texture = rb_vulkan_parameter_bind_texture,
                                                             .parameter_bind_target = rb_vulkan_parameter_bind_target,
                                                             .link_buffer = rb_vulkan_link_buffer,
                                                             .deallocate_buffer = rb_vulkan_deallocate_buffer,
                                                             .deallocate_shader = rb_vulkan_deallocate_shader,
                                                             .deallocate_program = rb_vulkan_deallocate_program,
                                                             .deallocate_texture = rb_vulkan_deallocate_texture,
                                                             .allocate_target = rb_vulkan_allocate_target,
                                                             .resize_target = rb_vulkan_resize_target,
                                                             .deallocate_target = rb_vulkan_deallocate_target};

render_backend_t*
render_backend_vulkan_allocate(void) {
	render_backend_t* backend =
	    memory_allocate(HASH_RENDER, sizeof(render_backend_vulkan_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_VULKAN;
	backend->api_group = RENDERAPIGROUP_VULKAN;
	backend->vtable = render_backend_vtable_null;
	return backend;
}

#else

render_backend_t*
render_backend_vulkan_allocate(void) {
	return 0;
}

#endif
