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

#if FOUNDATION_PLATFORM_WINDOWS
#include <foundation/windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#if FOUNDATION_PLATFORM_LINUX
#include <vulkan/vulkan_xlib.h>
#endif

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic push
#if __has_warning("-Wcast-align")
#pragma clang diagnostic ignored "-Wcast-align"
#endif
#endif

typedef struct render_adapter_vulkan_t {
	uint adapter_index;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	uint32_t queue_family_count;
	VkQueueFamilyProperties* queue_props;
	uint32_t queue_family_index;

	VkDevice device;
} render_adapter_vulkan_t;

typedef struct render_backend_vulkan_t {
	render_backend_t backend;

	VkInstance instance;

	VkPhysicalDevice* adapter_available;
	uint adapter_count;
	render_adapter_vulkan_t** adapter;
} render_backend_vulkan_t;

typedef struct render_target_vulkan_t {
	render_target_t target;
	VkFormat target_format;
} render_target_vulkan_t;

typedef struct render_target_window_vulkan_t {
	render_target_vulkan_t target;

	uint adapter_index;
	VkSurfaceKHR surface;
	VkCommandPool command_pool;
	VkSwapchainKHR swap_chain;
	VkImage* swap_chain_image;
	VkImageView* swap_chain_image_view;
} render_target_window_vulkan_t;

typedef struct render_pipeline_vulkan_t {
	render_pipeline_t pipeline;
	uint color_attachment_count;
	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkAttachmentLoadOp color_load_op[RENDER_TARGET_COLOR_ATTACHMENT_COUNT];
	vector_t color_clear[RENDER_TARGET_COLOR_ATTACHMENT_COUNT];
} render_pipeline_vulkan_t;

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

static bool
rb_vulkan_adapter_construct(render_backend_t* backend, render_adapter_vulkan_t* adapter, uint adapter_index,
                            VkSurfaceKHR surface) {
	render_backend_vulkan_t* backend_vk = (render_backend_vulkan_t*)backend;

	adapter->physical_device = backend_vk->adapter_available[adapter_index];

	vkGetPhysicalDeviceProperties(adapter->physical_device, &adapter->device_properties);
	vkGetPhysicalDeviceFeatures(adapter->physical_device, &adapter->device_features);

	vkGetPhysicalDeviceQueueFamilyProperties(adapter->physical_device, &adapter->queue_family_count, NULL);
	if ((int)adapter->queue_family_count < 1) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan adapter device, invalid queue count: %d"),
		           (int)adapter->queue_family_count);
		return false;
	}

	adapter->queue_props = memory_allocate(HASH_RENDER, adapter->queue_family_count * sizeof(VkQueueFamilyProperties),
	                                       0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	vkGetPhysicalDeviceQueueFamilyProperties(adapter->physical_device, &adapter->queue_family_count,
	                                         adapter->queue_props);

	// Find most suitable queue
	adapter->queue_family_index = UINT32_MAX;
	for (uint32_t iqueue = 0; iqueue < adapter->queue_family_count; ++iqueue) {
		VkBool32 supports_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(adapter->physical_device, iqueue, surface, &supports_present);
		if (((adapter->queue_props[iqueue].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) && supports_present) {
			adapter->queue_family_index = iqueue;
			break;
		}
	}
	if (adapter->queue_family_index == UINT32_MAX) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan adapter device, unable to find queue supporting both "
		                        "graphics and present"));
		return false;
	}

	float queue_priority = 0;
	VkDeviceQueueCreateInfo queue_info;
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.queueFamilyIndex = adapter->queue_family_index;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &queue_priority;
	queue_info.flags = 0;

	// Device extensions required
	uint32_t extension_count = 0;
	bool found_swapchain_ext = false;
#define MAX_EXTENSION_COUNT 16
	const char* required_extension[MAX_EXTENSION_COUNT];
	uint required_extension_count = 0;

	VkResult result = vkEnumerateDeviceExtensionProperties(adapter->physical_device, 0, &extension_count, 0);
	if (extension_count > 0) {
		VkExtensionProperties* device_extension =
		    memory_allocate(HASH_RENDER, sizeof(VkExtensionProperties) * extension_count, 0,
		                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		result = vkEnumerateDeviceExtensionProperties(adapter->physical_device, 0, &extension_count, device_extension);
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
		           STRING_CONST("Failed to create Vulkan adapter device, missing device extension %s"),
		           VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		return false;
	}

	VkDeviceCreateInfo device_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	                                  .pNext = 0,
	                                  .queueCreateInfoCount = 1,
	                                  .pQueueCreateInfos = &queue_info,
	                                  .enabledLayerCount = 0,
	                                  .ppEnabledLayerNames = 0,
	                                  .enabledExtensionCount = required_extension_count,
	                                  .ppEnabledExtensionNames = required_extension,
	                                  .pEnabledFeatures = 0};
	result = vkCreateDevice(adapter->physical_device, &device_info, 0, &adapter->device);
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Failed to create Vulkan adapter device: %d"),
		           (int)result);
		return false;
	}

	return true;
}

static void
rb_vulkan_adapter_destruct(render_backend_t* backend, render_adapter_vulkan_t* adapter) {
	FOUNDATION_UNUSED(backend);
	if (!adapter)
		return;
	if (adapter->device)
		vkDestroyDevice(adapter->device, nullptr);
	memory_deallocate(adapter->queue_props);
	memory_deallocate(adapter);
}

static void
rb_vulkan_destruct(render_backend_t* backend) {
	render_backend_vulkan_t* backend_vk = (render_backend_vulkan_t*)backend;

	for (uint iadapter = 0; iadapter < backend_vk->adapter_count; ++iadapter)
		rb_vulkan_adapter_destruct(backend, backend_vk->adapter[iadapter]);
	array_deallocate(backend_vk->adapter);

	if (backend_vk->adapter_available)
		memory_deallocate(backend_vk->adapter_available);
	backend_vk->adapter_available = 0;
	backend_vk->adapter_count = 0;

	log_debug(HASH_RENDER, STRING_CONST("Destructed Vulkan render backend"));
}

static size_t
rb_vulkan_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	render_backend_vulkan_t* backend_vk = (render_backend_vulkan_t*)backend;

	if (!backend_vk->instance) {
		if (!rb_vulkan_construct(backend))
			return 0;
	}

	if (!backend_vk->adapter_available) {
		backend_vk->adapter_count = 0;

		VkResult result = vkEnumeratePhysicalDevices(backend_vk->instance, &backend_vk->adapter_count, 0);
		if ((result != VK_SUCCESS) || (backend_vk->adapter_count == 0)) {
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Failed to enumerate Vulkan physical devices (%d)"), (int)result);
			return 0;
		}

		backend_vk->adapter_available =
		    memory_allocate(HASH_RENDER, sizeof(VkPhysicalDevice) * backend_vk->adapter_count, 0,
		                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		result =
		    vkEnumeratePhysicalDevices(backend_vk->instance, &backend_vk->adapter_count, backend_vk->adapter_available);
		if (result != VK_SUCCESS) {
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Failed to enumerate Vulkan physical devices (%d)"), (int)result);
			return 0;
		}

		array_resize(backend_vk->adapter, backend_vk->adapter_count);
		memset(backend_vk->adapter, 0, sizeof(render_adapter_vulkan_t*) * backend_vk->adapter_count);
	}

	// Arrange adapters in suitable order
	int priority_order[5] = {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
	                         VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, VK_PHYSICAL_DEVICE_TYPE_CPU,
	                         VK_PHYSICAL_DEVICE_TYPE_OTHER};
	uint iadapter = 0;
	for (uint itype = 0; (itype < 5) && (iadapter < capacity); ++itype) {
		VkPhysicalDeviceProperties properties;
		for (uint32_t idev = 0; (idev < backend_vk->adapter_count) && (iadapter < capacity); ++idev) {
			vkGetPhysicalDeviceProperties(backend_vk->adapter_available[idev], &properties);
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
		log_errorf(HASH_RENDER, ERROR_INVALID_VALUE,
		           STRING_CONST("Failed to create Vulkan window target, bad adapter index: %u"), window->adapter);
		return false;
	}

	uint adapter_index = (window->adapter != WINDOW_ADAPTER_DEFAULT) ? window->adapter : 0;
	VkPhysicalDevice physical_device = backend_vk->adapter_available[adapter_index];

	VkPhysicalDeviceProperties properties;
	static const char* device_type[] = {"other", "integrated GPU", "discrete GPU", "virtual GPU", "CPU"};
	vkGetPhysicalDeviceProperties(physical_device, &properties);
	log_infof(HASH_RENDER, STRING_CONST("Using Vulkan GPU %d: %s (%s)"), adapter_index, properties.deviceName,
	          device_type[math_clamp(properties.deviceType, 0, VK_PHYSICAL_DEVICE_TYPE_CPU + 1)]);

	render_target_window_vulkan_t* target_vk = memory_allocate(HASH_RENDER, sizeof(render_target_window_vulkan_t), 0,
	                                                           MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	render_target_t* target = (render_target_t*)target_vk;
	target->backend = backend;
	target->width = window_width(window);
	target->height = window_height(window);
	target->type = RENDERTARGET_WINDOW;
	target->pixelformat = PIXELFORMAT_R8G8B8A8;
	target->colorspace = COLORSPACE_sRGB;

	target_vk->adapter_index = adapter_index;

	// Create surface
#if FOUNDATION_PLATFORM_WINDOWS
	VkWin32SurfaceCreateInfoKHR surface_info;
	surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.hinstance = window->instance;
	surface_info.hwnd = window->hwnd;
	VkResult result = vkCreateWin32SurfaceKHR(backend_vk->instance, &surface_info, NULL, &target_vk->surface);
#elif FOUNDATION_PLATFORM_LINUX
	VkXlibSurfaceCreateInfoKHR surface_info;
	surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surface_info.pNext = NULL;
	surface_info.flags = 0;
	surface_info.dpy = window->display;
	surface_info.window = window->drawable;
	VkResult result = vkCreateXlibSurfaceKHR(backend_vk->instance, &surface_info, NULL, &target_vk->surface);
#endif
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to create surface: %d"), (int)result);
		memory_deallocate(target_vk);
		return false;
	}

	if (!backend_vk->adapter[adapter_index]) {
		backend_vk->adapter[adapter_index] = memory_allocate(HASH_RENDER, sizeof(render_adapter_vulkan_t), 0,
		                                                     MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		if (!rb_vulkan_adapter_construct(backend, backend_vk->adapter[adapter_index], adapter_index,
		                                 target_vk->surface))
			return false;
	}

	render_adapter_vulkan_t* adapter = backend_vk->adapter[adapter_index];

	VkCommandPoolCreateInfo command_pool_info = {0};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.queueFamilyIndex = adapter->queue_family_index;
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(adapter->device, &command_pool_info, nullptr, &target_vk->command_pool);
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to create command pool: %d"),
		           (int)result);
		render_target_deallocate(target);
		return false;
	}

	VkSurfaceFormatKHR* format = nullptr;
	uint32_t format_count = 0;
	uint32_t format_selected = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, target_vk->surface, &format_count, nullptr);
	if (!format_count) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to enumerate surface formats: %d"),
		           (int)result);
		render_target_deallocate(target);
		return false;
	}
	format = memory_allocate(HASH_RENDER, sizeof(VkSurfaceFormatKHR) * format_count, 0, MEMORY_TEMPORARY);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, target_vk->surface, &format_count, format);
	for (uint iformat = 0; iformat < format_count; ++iformat) {
		if ((format[iformat].format == VK_FORMAT_B8G8R8A8_SRGB) &&
		    (format[iformat].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
			format_selected = iformat;
			break;
		}
	}

	VkPresentModeKHR* present_mode = nullptr;
	uint32_t present_mode_count = 0;
	VkPresentModeKHR present_mode_selected = VK_PRESENT_MODE_FIFO_KHR;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, target_vk->surface, &present_mode_count, nullptr);
	if (!present_mode_count) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to enumerate surface present modes: %d"),
		           (int)result);
		render_target_deallocate(target);
		return false;
	}
	present_mode = memory_allocate(HASH_RENDER, sizeof(VkPresentModeKHR) * present_mode_count, 0, MEMORY_TEMPORARY);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, target_vk->surface, &present_mode_count, present_mode);
	for (uint imode = 0; imode < present_mode_count; ++imode) {
		if (present_mode[imode] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode_selected = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	memory_deallocate(present_mode);

	VkSurfaceCapabilitiesKHR surface_caps = {0};
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, target_vk->surface, &surface_caps);
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to query surface capabilities: %d"),
		           (int)result);
		memory_deallocate(format);
		render_target_deallocate(target);
		return false;
	}

	VkExtent2D extent = {.width = target->width, .height = target->height};
	extent.width = math_clamp(extent.width, surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width);
	extent.height = math_clamp(extent.height, surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);

	VkSwapchainCreateInfoKHR swapchain_info = {0};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = target_vk->surface;
	swapchain_info.minImageCount = surface_caps.minImageCount;
	swapchain_info.imageFormat = format[format_selected].format;
	swapchain_info.imageColorSpace = format[format_selected].colorSpace;
	swapchain_info.imageExtent = extent;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.queueFamilyIndexCount = 0;
	swapchain_info.pQueueFamilyIndices = nullptr;
	swapchain_info.preTransform = surface_caps.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = present_mode_selected;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;

	target_vk->target.target_format = format[format_selected].format;
	memory_deallocate(format);

	result = vkCreateSwapchainKHR(adapter->device, &swapchain_info, nullptr, &target_vk->swap_chain);
	if (result != VK_SUCCESS) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to create swap chain: %d"), (int)result);
		render_target_deallocate(target);
		return false;
	}

	uint32_t image_count = 0;
	result = vkGetSwapchainImagesKHR(adapter->device, target_vk->swap_chain, &image_count, nullptr);
	if (result == VK_SUCCESS) {
		array_resize(target_vk->swap_chain_image, image_count);
		result =
		    vkGetSwapchainImagesKHR(adapter->device, target_vk->swap_chain, &image_count, target_vk->swap_chain_image);
	}
	if (!image_count || (result != VK_SUCCESS)) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to create Vulkan window target, unable to get swap chain image count: %d"),
		           (int)result);
		render_target_deallocate(target);
		return false;
	}

	array_resize(target_vk->swap_chain_image_view, image_count);
	for (uint iimg = 0; iimg < image_count; ++iimg) {
		VkImageViewCreateInfo image_view_info = {0};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = target_vk->swap_chain_image[iimg];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = target_vk->target.target_format;
		image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;
		result = vkCreateImageView(adapter->device, &image_view_info, nullptr, &target_vk->swap_chain_image_view[iimg]);
		if (result != VK_SUCCESS) {
			log_errorf(
			    HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			    STRING_CONST("Failed to create Vulkan window target, unable to create swap chain image view: %d"),
			    (int)result);
			render_target_deallocate(target);
			return false;
		}
	}

	return target;
}

static render_target_t*
rb_vulkan_target_texture_allocate(render_backend_t* backend, uint width, uint height, render_pixelformat_t format) {
	FOUNDATION_UNUSED(backend, width, height, format);
	return 0;
}

static void
rb_vulkan_target_deallocate(render_backend_t* backend, render_target_t* target) {
	if (!target)
		return;
	render_backend_vulkan_t* backend_vk = (render_backend_vulkan_t*)backend;
	render_target_window_vulkan_t* target_vk = (render_target_window_vulkan_t*)target;
	render_adapter_vulkan_t* adapter = backend_vk->adapter[target_vk->adapter_index];
	for (uint iimg = 0, image_count = array_count(target_vk->swap_chain_image_view); iimg < image_count; ++iimg)
		vkDestroyImageView(adapter->device, target_vk->swap_chain_image_view[iimg], nullptr);
	array_deallocate(target_vk->swap_chain_image_view);
	array_deallocate(target_vk->swap_chain_image);
	if (target_vk->swap_chain)
		vkDestroySwapchainKHR(adapter->device, target_vk->swap_chain, nullptr);
	if (target_vk->command_pool)
		vkDestroyCommandPool(adapter->device, target_vk->command_pool, nullptr);
	memory_deallocate(target);
}

static render_pipeline_t*
rb_vulkan_pipeline_allocate(render_backend_t* backend, render_indexformat_t index_format, uint capacity) {
	render_pipeline_vulkan_t* pipeline_vk =
	    memory_allocate(HASH_RENDER, sizeof(render_pipeline_vulkan_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	render_pipeline_t* pipeline = (render_pipeline_t*)pipeline_vk;
	pipeline->backend = backend;
	pipeline->primitive_buffer =
	    render_buffer_allocate(backend, RENDERUSAGE_RENDER, sizeof(render_primitive_t) * capacity, 0, 0);
	pipeline->index_format = index_format;
	return pipeline;
}

static void
rb_vulkan_pipeline_deallocate(render_backend_t* backend, render_pipeline_t* pipeline) {
	render_backend_vulkan_t* backend_vk = (render_backend_vulkan_t*)backend;
	render_pipeline_vulkan_t* pipeline_vk = (render_pipeline_vulkan_t*)pipeline;
	if (pipeline) {
		render_buffer_deallocate(pipeline->primitive_buffer);

		FOUNDATION_UNUSED(backend_vk);
		FOUNDATION_UNUSED(pipeline_vk);
		// vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		// vkDestroyRenderPass(device, renderPass, nullptr);
	}
	memory_deallocate(pipeline);
}

static void
rb_vulkan_pipeline_set_color_attachment(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                        render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_vulkan_t* pipeline_vk = (render_pipeline_vulkan_t*)pipeline;
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT) {
		pipeline->color_attachment[slot] = target;
		if (pipeline_vk->color_attachment_count <= slot)
			pipeline_vk->color_attachment_count = slot + 1;
	}
}

static void
rb_vulkan_pipeline_set_depth_attachment(render_backend_t* backend, render_pipeline_t* pipeline,
                                        render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	pipeline->depth_attachment = target;
}

static void
rb_vulkan_pipeline_set_color_clear(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                   render_clear_action_t action, vector_t color) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_vulkan_t* pipeline_vk = (render_pipeline_vulkan_t*)pipeline;
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT) {
		switch (action) {
			case RENDERCLEAR_CLEAR:
				pipeline_vk->color_load_op[slot] = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case RENDERCLEAR_PRESERVE:
				pipeline_vk->color_load_op[slot] = VK_ATTACHMENT_LOAD_OP_LOAD;
				break;
			default:
			case RENDERCLEAR_DONTCARE:
				pipeline_vk->color_load_op[slot] = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
		}
		pipeline_vk->color_clear[slot] = color;
	}
}

static void
rb_vulkan_pipeline_set_depth_clear(render_backend_t* backend, render_pipeline_t* pipeline, render_clear_action_t action,
                                   vector_t color) {
	FOUNDATION_UNUSED(backend, pipeline, action, color);
}

static void
rb_vulkan_pipeline_build(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_vulkan_t* pipeline_vk = (render_pipeline_vulkan_t*)pipeline;
	VkAttachmentDescription color_attachment_desc[RENDER_TARGET_COLOR_ATTACHMENT_COUNT];
	for (uint islot = 0; islot < pipeline_vk->color_attachment_count; ++islot) {
		render_target_vulkan_t* target_vk = (render_target_vulkan_t*)pipeline->color_attachment[islot];
		if (!target_vk)
			continue;
		color_attachment_desc[islot].format = target_vk->target_format;
		color_attachment_desc[islot].samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment_desc[islot].loadOp = pipeline_vk->color_load_op[islot];
		color_attachment_desc[islot].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment_desc[islot].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment_desc[islot].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment_desc[islot].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (islot == 0)
			color_attachment_desc[islot].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		else
			color_attachment_desc[islot].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference color_attachment_ref[RENDER_TARGET_COLOR_ATTACHMENT_COUNT];
	for (uint islot = 0; islot < pipeline_vk->color_attachment_count; ++islot) {
		color_attachment_ref[islot].attachment = islot;
		color_attachment_ref[islot].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = pipeline_vk->color_attachment_count;
	subpass.pColorAttachments = color_attachment_ref;
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
    .pipeline_build = rb_vulkan_pipeline_build,
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
