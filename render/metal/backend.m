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

#include <render/metal/backend.h>

#if FOUNDATION_PLATFORM_APPLE

#include <Metal/Metal.h>

#import <QuartzCore/QuartzCore.h>

typedef struct render_backend_metal_t {
	RENDER_DECLARE_BACKEND;

	id<MTLDevice> device;
	CAMetalLayer* layer;
	id<MTLCommandQueue> command_queue;
	MTLRenderPassDescriptor render_pass_descriptor;
} render_backend_metal_t;

static bool
rb_metal_construct(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	backend_metal->device = MTLCreateSystemDefaultDevice();
	if (!backend_metal->device) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create default system metal device"));
		return false;
	}

	log_info(HASH_RENDER, STRING_CONST("Constructed metal render backend"));
	return true;
}

static void
rb_metal_destruct(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	backend_metal->device = 0;
	log_info(HASH_RENDER, STRING_CONST("Destructed metal render backend"));
}

static size_t
rb_metal_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	if (capacity)
		store[0] = (unsigned int)WINDOW_ADAPTER_DEFAULT;
	return 1;
}

static size_t
rb_metal_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(adapter);
	if (capacity) {
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, COLORSPACE_LINEAR, 60};
		store[0] = mode;
	}
	return 1;
}

static bool
rb_metal_set_drawable(render_backend_t* backend, const render_drawable_t* drawable) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	if (!drawable || !drawable->view)
        return false;
    
	dispatch_sync(dispatch_get_main_queue(), ^{
		backend_metal->layer = (CAMetalLayer*)((__bridge NSView*)drawable->view).layer;
	});
	if (!backend_metal->layer)
		return false;

	backend_metal->command_queue = [backend_metal->device newCommandQueue];
	backend_metal->render_pass_descriptor = [MTLRenderPassDescriptor new];
    return true;
}

static void
rb_metal_dispatch(render_backend_t* backend, render_target_t* target, render_context_t** contexts,
                  size_t contexts_count) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	if (!backend_metal->layer)
		return;
	
	/*
	for (size_t cindex = 0, csize = contexts_count; cindex < csize; ++cindex) {
		render_context_t* context = contexts[cindex];
		int cmd_size = atomic_load32(&context->reserved, memory_order_acquire);
		if (context->sort->indextype == RADIXSORT_INDEX16) {
			const uint16_t* order = context->order;
			for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order)
				rb_gl4_dispatch_command(backend_gl4, target, context, context->commands + *order);
		} else if (context->sort->indextype == RADIXSORT_INDEX32) {
			const uint32_t* order = context->order;
			for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order)
				rb_gl4_dispatch_command(backend_gl4, target, context, context->commands + *order);
		}
	}

	backend_metal->render_pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
	backend_metal->render_pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
	backend_metal->render_pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 1, 1, 1);
	 */
	FOUNDATION_UNUSED(target);
	FOUNDATION_UNUSED(contexts);
	FOUNDATION_UNUSED(contexts_count);
}

static void
rb_metal_flip(render_backend_t* backend) {
	++backend->framecount;
}

static void*
rb_metal_allocate_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	return memory_allocate(HASH_RENDER, buffer->buffersize, 16, MEMORY_PERSISTENT);
}

static void
rb_metal_deallocate_buffer(render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(aux);
	if (sys)
		memory_deallocate(buffer->store);
}

static bool
rb_metal_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	return true;
}

static bool
rb_metal_upload_shader(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(shader);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	return true;
}

static bool
rb_metal_upload_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(program);
	return true;
}

static bool
rb_metal_upload_texture(render_backend_t* backend, render_texture_t* texture, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(texture);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	return true;
}

static void
rb_metal_deallocate_texture(render_backend_t* backend, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(texture);
}

static void
rb_metal_parameter_bind_texture(render_backend_t* backend, void* buffer, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(texture);
}

static void
rb_metal_parameter_bind_target(render_backend_t* backend, void* buffer, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(target);
}

static void
rb_metal_link_buffer(render_backend_t* backend, render_buffer_t* buffer, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(program);
}

static void
rb_metal_deallocate_shader(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(shader);
}

static void
rb_metal_deallocate_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(program);
}

static bool
rb_metal_allocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
	return true;
}

static bool
rb_metal_resize_target(render_backend_t* backend, render_target_t* target, unsigned int width, unsigned int height) {
	FOUNDATION_UNUSED(backend);
	if (target) {
		target->width = width;
		target->height = height;
	}
	return true;
}

static void
rb_metal_deallocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
}

static void
rb_metal_enable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static void
rb_metal_disable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static render_backend_vtable_t render_backend_vtable_metal = {.construct = rb_metal_construct,
                                                              .destruct = rb_metal_destruct,
                                                              .enumerate_adapters = rb_metal_enumerate_adapters,
                                                              .enumerate_modes = rb_metal_enumerate_modes,
                                                              .set_drawable = rb_metal_set_drawable,
                                                              .enable_thread = rb_metal_enable_thread,
                                                              .disable_thread = rb_metal_disable_thread,
                                                              .dispatch = rb_metal_dispatch,
                                                              .flip = rb_metal_flip,
                                                              .allocate_buffer = rb_metal_allocate_buffer,
                                                              .upload_buffer = rb_metal_upload_buffer,
                                                              .upload_shader = rb_metal_upload_shader,
                                                              .upload_program = rb_metal_upload_program,
                                                              .upload_texture = rb_metal_upload_texture,
                                                              .parameter_bind_texture = rb_metal_parameter_bind_texture,
                                                              .parameter_bind_target = rb_metal_parameter_bind_target,
                                                              .link_buffer = rb_metal_link_buffer,
                                                              .deallocate_buffer = rb_metal_deallocate_buffer,
                                                              .deallocate_shader = rb_metal_deallocate_shader,
                                                              .deallocate_program = rb_metal_deallocate_program,
                                                              .deallocate_texture = rb_metal_deallocate_texture,
                                                              .allocate_target = rb_metal_allocate_target,
                                                              .resize_target = rb_metal_resize_target,
                                                              .deallocate_target = rb_metal_deallocate_target};

render_backend_t*
render_backend_metal_allocate(void) {
	render_backend_t* backend =
	    memory_allocate(HASH_RENDER, sizeof(render_backend_metal_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_METAL;
	backend->api_group = RENDERAPIGROUP_METAL;
	backend->vtable = render_backend_vtable_metal;
	return backend;
}

#endif
