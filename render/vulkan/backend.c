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
#include <vulkan/vulkan_xlib.h>
#endif

typedef struct render_backend_vulkan_t {
	render_backend_t backend;

	VkInstance instance;
	
	VkPhysicalDevice* adapter_available;
	uint adapter_count;
} render_backend_vulkan_t;

typedef struct render_target_window_vulkan_t {
	render_target_t target;

	VkPhysicalDevice adapter;
	VkPhysicalDeviceProperties adapter_properties;
	VkPhysicalDeviceFeatures adapter_features;

	uint32_t queue_family_count;
	VkQueueFamilyProperties* queue_props;
	uint32_t queue_family_index;

	VkDevice device;
	VkSurfaceKHR surface;
} render_target_window_vulkan_t;

static bool
rb_vulkan_construct(render_backend_t* backend) {
	render_backend_vulkan_t* backend_vulkan = (render_backend_vulkan_t*)backend;
	if (backend_vulkan->instance)
		return true;

	const application_t* app = environment_application();

	VkApplicationInfo vk_app_info = {0};
	vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_app_info.pApplicationName = app->name.str;
	vk_app_info.applicationVersion =
	    VK_MAKE_VERSION(app->version.sub.major, app->version.sub.minor, app->version.sub.revision);
	vk_app_info.pEngineName = "Neoengine";
	vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);

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
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
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
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, 60};
		store[0] = mode;
	}
	return 1;
}

static render_target_t*
rb_vulkan_target_window_allocate(render_backend_t* backend, window_t* window, uint tag) {
	FOUNDATION_UNUSED(tag);
	render_backend_vulkan_t* backend_vk = (render_backend_vulkan_t*)backend;

	if (!backend_vk->adapter_available)
		rb_vulkan_enumerate_adapters(backend, 0, 0);
	if ((window->adapter != WINDOW_ADAPTER_DEFAULT) && (window->adapter >= backend_vk->adapter_count)) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Failed to create Vulkan window target, bad adapter index"));
		return false;
	}

	uint adapter_index = (window->adapter != WINDOW_ADAPTER_DEFAULT) ? window->adapter : 0;
	VkPhysicalDevice adapter = backend_vk->adapter_available[adapter_index];

	VkPhysicalDeviceProperties properties;
	static const char* device_type[] = {
	    "other",
	    "integrated GPU",
	    "discrete GPU",
	    "virtual GPU",
	    "CPU"
	};
	vkGetPhysicalDeviceProperties(adapter, &properties);
	log_infof(HASH_RENDER, STRING_CONST("Using Vulkan GPU %d: %s (%s)"), adapter_index, properties.deviceName,
	          device_type[math_clamp(properties.deviceType, 0, VK_PHYSICAL_DEVICE_TYPE_CPU + 1)]);

	// Device extensions required
	uint32_t extension_count = 0;
	bool found_swapchain_ext = false;
#define MAX_EXTENSION_COUNT 16
	const char* required_extension[MAX_EXTENSION_COUNT];
	uint required_extension_count = 0;

	VkResult result = vkEnumerateDeviceExtensionProperties(adapter, 0, &extension_count, 0);
	if (extension_count > 0) {
		VkExtensionProperties* device_extension =
		    memory_allocate(HASH_RENDER, sizeof(VkExtensionProperties) * extension_count, 0,
		                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		result = vkEnumerateDeviceExtensionProperties(adapter, 0, &extension_count, device_extension);
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
		           STRING_CONST("Failed to create Vulkan window target, missing device extension %s"),
		           VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		return false;
	}

	render_target_window_vulkan_t* target_vk = memory_allocate(HASH_RENDER, sizeof(render_target_window_vulkan_t), 0,
	                                                           MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	target_vk->adapter = adapter;

	render_target_t* target = (render_target_t*)target_vk;
	target->backend = backend;
	target->width = window_width(window);
	target->height = window_height(window);
	target->type = RENDERTARGET_WINDOW;
	target->pixelformat = PIXELFORMAT_R8G8B8A8;
	target->colorspace = COLORSPACE_sRGB;

	// Create surface
#if FOUNDATION_PLATFORM_WINDOWS
	VkWin32SurfaceCreateInfoKHR surface_info;
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.hinstance = window->instance;
	surface_info.hwnd = window->hwnd;
	result = vkCreateWin32SurfaceKHR(backend_vk->instance, &surface_info, NULL, &target_vk->surface);
#elif FOUNDATION_PLATFORM_LINUX
	VkXlibSurfaceCreateInfoKHR surface_info;
	surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.dpy = window->display;
	surface_info.window = window->drawable;
	result = vkCreateXlibSurfaceKHR(backend_vk->instance, &surface_info, NULL, &target_vk->surface);
#endif
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to create surface: %d"),
		           (int)result);
		memory_deallocate(target_vk);
		return false;
	}

	// Initialize swapchain
	vkGetPhysicalDeviceProperties(target_vk->adapter, &target_vk->adapter_properties);
	vkGetPhysicalDeviceFeatures(target_vk->adapter, &target_vk->adapter_features);

	vkGetPhysicalDeviceQueueFamilyProperties(target_vk->adapter, &target_vk->queue_family_count, NULL);
	if ((int)target_vk->queue_family_count < 1) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, invalid device queue count: %d"),
		           (int)target_vk->queue_family_count);
		render_target_deallocate(target);
		return false;
	}

	target_vk->queue_props =
	    memory_allocate(HASH_RENDER, target_vk->queue_family_count * sizeof(VkQueueFamilyProperties), 0,
	                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	vkGetPhysicalDeviceQueueFamilyProperties(target_vk->adapter, &target_vk->queue_family_count,
	                                         target_vk->queue_props);

	// Find most suitable queue
	target_vk->queue_family_index = UINT32_MAX;
	for (uint32_t iqueue = 0; iqueue < target_vk->queue_family_count; ++iqueue) {
		VkBool32 supports_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(target_vk->adapter, iqueue, target_vk->surface, &supports_present);
		if (((target_vk->queue_props[iqueue].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) && supports_present) {
			target_vk->queue_family_index = iqueue;
			break;
		}
	}
	if (target_vk->queue_family_index == UINT32_MAX) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to find queue supporting both graphics and present"));
		render_target_deallocate(target);
		return false;
	}

    float queue_priorities[1] = {0.0};
	VkDeviceQueueCreateInfo queues[2];
	queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queues[0].pNext = NULL;
	queues[0].queueFamilyIndex = target_vk->queue_family_index;
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
		.enabledExtensionCount = required_extension_count,
	    .ppEnabledExtensionNames = required_extension,
	    .pEnabledFeatures = 0
	};
	result = vkCreateDevice(target_vk->adapter, &device_info, 0, &target_vk->device);

	return target;
}

static render_target_t*
rb_vulkan_target_texture_allocate(render_backend_t* backend, uint width, uint height, render_pixelformat_t format) {
	FOUNDATION_UNUSED(backend, width, height, format);
	return 0;
}

static void
rb_vulkan_target_deallocate(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	memory_deallocate(target);
}

static render_pipeline_t*
rb_vulkan_pipeline_allocate(render_backend_t* backend, render_indexformat_t index_format, uint capacity) {
	render_pipeline_t* pipeline =
	    memory_allocate(HASH_RENDER, sizeof(render_pipeline_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	pipeline->backend = backend;
	pipeline->primitive_buffer = render_buffer_allocate(backend, RENDERUSAGE_RENDER, sizeof(render_primitive_t) * capacity, 0, 0);
	pipeline->index_format = index_format;
	return pipeline;
}

static void
rb_vulkan_pipeline_deallocate(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend);
	if (pipeline)
		render_buffer_deallocate(pipeline->primitive_buffer);
	memory_deallocate(pipeline);
}

static void
rb_vulkan_pipeline_set_color_attachment(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                      render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT)
		pipeline->color_attachment[slot] = target;
}

static void
rb_vulkan_pipeline_set_depth_attachment(render_backend_t* backend, render_pipeline_t* pipeline, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	pipeline->depth_attachment = target;
}

static void
rb_vulkan_pipeline_set_color_clear(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                 render_clear_action_t action, vector_t color) {
	FOUNDATION_UNUSED(backend, pipeline, slot, action, color);
}

static void
rb_vulkan_pipeline_set_depth_clear(render_backend_t* backend, render_pipeline_t* pipeline, render_clear_action_t action,
                                 vector_t color) {
	FOUNDATION_UNUSED(backend, pipeline, action, color);
}

static void
rb_vulkan_pipeline_flush(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend, pipeline);
}

static void
rb_vulkan_pipeline_use_argument_buffer(render_backend_t* backend, render_pipeline_t* pipeline,
                                     render_buffer_index_t buffer) {
	FOUNDATION_UNUSED(backend, pipeline, buffer);
}

static void
rb_vulkan_pipeline_use_render_buffer(render_backend_t* backend, render_pipeline_t* pipeline,
                                   render_buffer_index_t buffer) {
	FOUNDATION_UNUSED(backend, pipeline, buffer);
}

static render_pipeline_state_t
rb_vulkan_pipeline_state_allocate(render_backend_t* backend, render_pipeline_t* pipeline, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend, pipeline, shader);
	return 0;
}

static void
rb_vulkan_pipeline_state_deallocate(render_backend_t* backend, render_pipeline_state_t state) {
	FOUNDATION_UNUSED(backend, state);
}

static bool
rb_vulkan_shader_upload(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend, shader, buffer, size);
	return true;
}

static void
rb_vulkan_shader_finalize(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend, shader);
}

static void
rb_vulkan_buffer_allocate(render_backend_t* backend, render_buffer_t* buffer, size_t buffer_size, const void* data,
                        size_t data_size) {
	FOUNDATION_UNUSED(backend);
	if (buffer->usage == RENDERUSAGE_GPUONLY)
		return;
	buffer->store = memory_allocate(HASH_RENDER, buffer_size, 0, MEMORY_PERSISTENT);
	buffer->allocated = buffer_size;
	if (data_size && buffer->store) {
		memcpy(buffer->store, data, data_size);
		buffer->used = data_size;
	}
}

static void
rb_vulkan_buffer_deallocate(render_backend_t* backend, render_buffer_t* buffer, bool cpu, bool gpu) {
	FOUNDATION_UNUSED(backend, gpu);
	if (cpu && buffer->store) {
		memory_deallocate(buffer->store);
		buffer->store = nullptr;
	}
}

static void
rb_vulkan_buffer_upload(render_backend_t* backend, render_buffer_t* buffer, size_t offset, size_t size) {
	FOUNDATION_UNUSED(backend, buffer, offset, size);
}

static void
rb_vulkan_buffer_data_declare(render_backend_t* backend, render_buffer_t* buffer, size_t instance_count,
                            const render_buffer_data_t* data, size_t data_count) {
	FOUNDATION_UNUSED(backend, buffer, data, data_count, instance_count);
}

static void
rb_vulkan_buffer_data_encode_buffer(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                  render_buffer_t* source, uint offset) {
	FOUNDATION_UNUSED(backend, buffer, instance, index, source, offset);
}

static void
rb_vulkan_buffer_data_encode_constant(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                    const void* data, uint size) {
	FOUNDATION_UNUSED(backend, buffer, instance, index, data, size);
}

static void
rb_vulkan_buffer_data_encode_matrix(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                  const matrix_t* matrix) {
	FOUNDATION_UNUSED(backend, buffer, instance, index, matrix);
}

static void
rb_vulkan_buffer_set_label(render_backend_t* backend, render_buffer_t* buffer, const char* name, size_t length) {
	FOUNDATION_UNUSED(backend, buffer, name, length);
}

static render_backend_vtable_t render_backend_vtable_null = {
	.construct = rb_vulkan_construct,
	.destruct = rb_vulkan_destruct,
	.enumerate_adapters = rb_vulkan_enumerate_adapters,
	.enumerate_modes = rb_vulkan_enumerate_modes,
	.target_window_allocate = rb_vulkan_target_window_allocate,
	.target_texture_allocate = rb_vulkan_target_texture_allocate,
	.target_deallocate = rb_vulkan_target_deallocate,
    .pipeline_allocate = rb_vulkan_pipeline_allocate,
    .pipeline_deallocate = rb_vulkan_pipeline_deallocate,
    .pipeline_set_color_attachment = rb_vulkan_pipeline_set_color_attachment,
    .pipeline_set_depth_attachment = rb_vulkan_pipeline_set_depth_attachment,
    .pipeline_set_color_clear = rb_vulkan_pipeline_set_color_clear,
    .pipeline_set_depth_clear = rb_vulkan_pipeline_set_depth_clear,
    .pipeline_flush = rb_vulkan_pipeline_flush,
    .pipeline_use_argument_buffer = rb_vulkan_pipeline_use_argument_buffer,
    .pipeline_use_render_buffer = rb_vulkan_pipeline_use_render_buffer,
    .pipeline_state_allocate = rb_vulkan_pipeline_state_allocate,
    .pipeline_state_deallocate = rb_vulkan_pipeline_state_deallocate,
    .shader_upload = rb_vulkan_shader_upload,
    .shader_finalize = rb_vulkan_shader_finalize,
    .buffer_allocate = rb_vulkan_buffer_allocate,
    .buffer_deallocate = rb_vulkan_buffer_deallocate,
    .buffer_upload = rb_vulkan_buffer_upload,
    .buffer_set_label = rb_vulkan_buffer_set_label,
    .buffer_data_declare = rb_vulkan_buffer_data_declare,
    .buffer_data_encode_buffer = rb_vulkan_buffer_data_encode_buffer,
    .buffer_data_encode_matrix = rb_vulkan_buffer_data_encode_matrix,
    .buffer_data_encode_constant = rb_vulkan_buffer_data_encode_constant};

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
