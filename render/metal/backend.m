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
#include <vector/vector.h>
#include <render/render.h>
#include <render/internal.h>

#include <render/metal/backend.h>

#if FOUNDATION_PLATFORM_APPLE

#include <Metal/Metal.h>

#import <QuartzCore/QuartzCore.h>

typedef struct render_backend_metal_t {
	render_backend_t backend;

	id<MTLDevice> device;
	//id<MTLCommandQueue> command_queue;
	//MTLRenderPassDescriptor* render_pass_descriptor;
} render_backend_metal_t;

typedef struct render_target_window_metal_t {
	render_target_t target;
	CAMetalLayer* metal_layer;
} render_target_window_metal_t;

typedef struct render_pipeline_metal_t {
	render_pipeline_t pipeline;
	MTLRenderPassDescriptor* descriptor;
	id<MTLCommandQueue> command_queue;
} render_pipeline_metal_t;

static void
rb_metal_destruct(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	@autoreleasepool {
		backend_metal->device = 0;
	}
	log_info(HASH_RENDER, STRING_CONST("Destructed metal render backend"));
}

static bool
rb_metal_construct(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	backend->shader_type = HASH_SHADER_METAL;

	backend_metal->device = MTLCreateSystemDefaultDevice();
	if (!backend_metal->device) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create default system metal device"));
		return false;
	}

	MTLArgumentBuffersTier argument_buffers_tier = [backend_metal->device argumentBuffersSupport];
	if (argument_buffers_tier == MTLArgumentBuffersTier1) {
		rb_metal_destruct(backend);
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Required Metal argument buffers tier 2 not supported by device"));
		return false;
	}

	log_info(HASH_RENDER, STRING_CONST("Constructed metal render backend"));
	return true;
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
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, 60};
		store[0] = mode;
	}
	return 1;
}

static render_target_t* 
rb_metal_target_window_allocate(render_backend_t* backend, window_t* window, uint tag) {
	void* view = window_view(window, tag);
	if (!view)
		return 0;

	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	render_target_window_metal_t* target_metal = memory_allocate(HASH_RENDER, sizeof(render_target_window_metal_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	render_target_t* target = (render_target_t*)target_metal;
	target->backend = backend;
	target->width = window_width(window);
	target->height = window_height(window);
	target->type = RENDERTARGET_WINDOW;
	target->pixelformat = PIXELFORMAT_R8G8B8A8;
	target->colorspace = COLORSPACE_sRGB;

	dispatch_sync(dispatch_get_main_queue(), ^{
	  target_metal->metal_layer = (CAMetalLayer*)((__bridge NSView*)view).layer;
	});
	if (!target_metal->metal_layer) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Unable to create Metal render target for window, view has no Metal layer"));
		memory_deallocate(target_metal);
		return 0;
	}

	target_metal->metal_layer.device = backend_metal->device;
	target_metal->metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

	return target;
}

static void
rb_metal_target_deallocate(render_backend_t* backend, render_target_t* target) {
	if (!target || (target->backend != backend))
		return;

	if (target->type == RENDERTARGET_WINDOW) {
		// Window target
	}

	memory_deallocate(target);
}

static render_pipeline_t*
rb_metal_pipeline_allocate(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	render_pipeline_metal_t* pipeline_metal = memory_allocate(HASH_RENDER, sizeof(render_pipeline_metal_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	
	render_pipeline_t* pipeline = (render_pipeline_t*)pipeline_metal;
	pipeline->backend = backend;

	pipeline_metal->descriptor = [MTLRenderPassDescriptor new];
	if (!pipeline_metal->descriptor) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create Metal render pass descriptor"));
		memory_deallocate(pipeline_metal);
		return 0;
	}

	pipeline_metal->command_queue = [backend_metal->device newCommandQueue];
	if (!pipeline_metal->command_queue) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create Metal render pass command queue"));
		memory_deallocate(pipeline_metal);
		return 0;
	}

	return pipeline;
}

static void
rb_metal_pipeline_deallocate(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;
	@autoreleasepool {
		pipeline_metal->descriptor = 0;
		pipeline_metal->command_queue = 0;
	}
	memory_deallocate(pipeline);
}

static void
rb_metal_pipeline_set_color_attachment(render_backend_t* backend, render_pipeline_t* pipeline, uint slot, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT)
		pipeline->color_attachment[slot] = target;
}

static void
rb_metal_pipeline_set_depth_attachment(render_backend_t* backend, render_pipeline_t* pipeline, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	pipeline->depth_attachment = target;
}

static void
rb_metal_pipeline_set_color_clear(render_backend_t* backend, render_pipeline_t* pipeline, uint slot, render_clear_action_t action, vector_t color) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT) {
		if (action == RENDERCLEAR_CLEAR) {
			pipeline_metal->descriptor.colorAttachments[slot].loadAction = MTLLoadActionClear;
			pipeline_metal->descriptor.colorAttachments[slot].clearColor = MTLClearColorMake((double)vector_x(color), (double)vector_y(color), (double)vector_z(color), (double)vector_w(color));
		} else if (action == RENDERCLEAR_PRESERVE) {
			pipeline_metal->descriptor.colorAttachments[slot].loadAction = MTLLoadActionLoad;
		} else {
			pipeline_metal->descriptor.colorAttachments[slot].loadAction = MTLLoadActionDontCare;
		}
	}
}

static void
rb_metal_pipeline_set_depth_clear(render_backend_t* backend, render_pipeline_t* pipeline, render_clear_action_t action, vector_t color) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;
	if (action == RENDERCLEAR_CLEAR) {
		pipeline_metal->descriptor.depthAttachment.loadAction = MTLLoadActionClear;
		pipeline_metal->descriptor.depthAttachment.clearDepth = (double)vector_x(color);
	} else if (action == RENDERCLEAR_PRESERVE) {
		pipeline_metal->descriptor.depthAttachment.loadAction = MTLLoadActionLoad;
	} else {
		pipeline_metal->descriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
	}
}

static void
rb_metal_pipeline_flush(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;

	@autoreleasepool {
		id<CAMetalDrawable> current_drawable = 0;

		render_target_t* target = pipeline->color_attachment[0];
		if (target && (target->type == RENDERTARGET_WINDOW)) {
			render_target_window_metal_t* target_window = (render_target_window_metal_t*)pipeline->color_attachment[0];
			current_drawable = [target_window->metal_layer nextDrawable];
		}

		MTLRenderPassDescriptor* desc = pipeline_metal->descriptor;
		desc.colorAttachments[0].texture = current_drawable.texture;

		id<MTLCommandBuffer> command_buffer = [pipeline_metal->command_queue commandBuffer];

		id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor:desc];

		MTLViewport viewport;
		viewport.originX = 0;
		viewport.originY = 0;
		viewport.width = 100;
		viewport.height = 100;
		viewport.znear = 0;
		viewport.zfar = 1;
		[render_encoder setViewport:viewport];

		[render_encoder endEncoding];

		[command_buffer presentDrawable:current_drawable];
		[command_buffer commit];
	}
}

static bool
rb_metal_shader_upload(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend, shader, buffer, size);
	return true;
}

static void
rb_metal_shader_finalize(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend, shader);
}

#if 0
static bool
rb_metal_set_drawable(render_backend_t* backend, const render_drawable_t* drawable) {
}

static void
rb_metal_clear(render_backend_metal_t* backend, id<MTLRenderCommandEncoder> render_encoder, render_context_t* context,
               render_command_t* command) {
	FOUNDATION_UNUSED(render_encoder, context);
	MTLRenderPassDescriptor* desc = backend->render_pass_descriptor;

	unsigned int buffer_mask = command->data.clear.buffer_mask;

	if (buffer_mask & RENDERBUFFER_COLOR) {
		uint32_t color = command->data.clear.color;
		desc.colorAttachments[0].loadAction = MTLLoadActionClear;
		desc.colorAttachments[0].clearColor =
		    MTLClearColorMake((double)(color & 0xFF) / 255.0, (double)((color >> 8) & 0xFF) / 255.0,
		                      (double)((color >> 16) & 0xFF) / 255.0, (double)((color >> 24) & 0xFF) / 255.0);
	}
	if (buffer_mask & RENDERBUFFER_DEPTH) {
		desc.depthAttachment.loadAction = MTLLoadActionClear;
		desc.depthAttachment.clearDepth = (double)command->data.clear.depth;
	}
}

static void
rb_metal_viewport(render_backend_metal_t* backend, id<MTLRenderCommandEncoder> render_encoder, render_target_t* target,
                  render_context_t* context, render_command_t* command) {
	FOUNDATION_UNUSED(backend, target, context);
	MTLViewport viewport;
	viewport.originX = command->data.viewport.x;
	viewport.originY = command->data.viewport.y;
	viewport.width = command->data.viewport.width;
	viewport.height = command->data.viewport.height;
	viewport.znear = (double)command->data.viewport.min_z;
	viewport.zfar = (double)command->data.viewport.max_z;
	[render_encoder setViewport:viewport];
}

static void
rb_metal_dispatch_command(render_backend_metal_t* backend, id<MTLRenderCommandEncoder> render_encoder,
                          render_target_t* target, render_context_t* context, render_command_t* command) {
	switch (command->type) {
		case RENDERCOMMAND_CLEAR:
			rb_metal_clear(backend, render_encoder, context, command);
			break;

		case RENDERCOMMAND_VIEWPORT:
			rb_metal_viewport(backend, render_encoder, target, context, command);
			break;

		case RENDERCOMMAND_RENDER_TRIANGLELIST:
		case RENDERCOMMAND_RENDER_LINELIST:
			// rb_metal_render(backend, context, command);
			break;
	}
}

static void
rb_metal_dispatch(render_backend_t* backend, render_target_t* target, render_context_t** contexts,
                  size_t contexts_count) {
	FOUNDATION_UNUSED(target, contexts, contexts_count);
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	if (!backend_metal->metal_layer)
		return;

	@autoreleasepool {
		id<CAMetalDrawable> current_drawable = [backend_metal->metal_layer nextDrawable];
		if (!current_drawable)
			return;

		id<MTLCommandBuffer> command_buffer = [backend_metal->command_queue commandBuffer];

		MTLRenderPassDescriptor* desc = backend_metal->render_pass_descriptor;
		desc.colorAttachments[0].texture = current_drawable.texture;

		id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor:desc];

		desc.colorAttachments[0].loadAction = MTLLoadActionDontCare;
		desc.depthAttachment.loadAction = MTLLoadActionDontCare;

		for (size_t cindex = 0, csize = contexts_count; cindex < csize; ++cindex) {
			render_context_t* context = contexts[cindex];
			int cmd_size = atomic_load32(&context->reserved, memory_order_acquire);
			if (context->sort->indextype == RADIXSORT_INDEX16) {
				const uint16_t* order = context->order;
				for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order)
					rb_metal_dispatch_command(backend_metal, render_encoder, target, context,
					                          context->commands + *order);
			} else if (context->sort->indextype == RADIXSORT_INDEX32) {
				const uint32_t* order = context->order;
				for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order)
					rb_metal_dispatch_command(backend_metal, render_encoder, target, context,
					                          context->commands + *order);
			}
		}

		[render_encoder endEncoding];

		[command_buffer presentDrawable:current_drawable];
		[command_buffer commit];
	}
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
	if (sys)
		memory_deallocate(buffer->store);
	if (aux) {
		if (buffer->backend_data[0]) {
			@autoreleasepool {
				id<MTLBuffer> buffer_metal;
				buffer_metal = (__bridge_transfer id<MTLBuffer>)((void*)buffer->backend_data[0]);
			}
		}
	}
}

static bool
rb_metal_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	if ((buffer->buffertype == RENDERBUFFER_PARAMETER) || (buffer->buffertype == RENDERBUFFER_STATE))
		return true;

	if (buffer->backend_data[0]) {
		@autoreleasepool {
			id<MTLBuffer> buffer_metal;
			buffer_metal = (__bridge_transfer id<MTLBuffer>)((void*)buffer->backend_data[0]);
		}
	}

	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	id<MTLBuffer> buffer_metal = [backend_metal->device newBufferWithBytes:buffer->store
	                                                                length:buffer->buffersize
	                                                               options:MTLResourceStorageModeShared];
	buffer->backend_data[0] = (uintptr_t)((__bridge_retained void*)buffer_metal);
	buffer->flags &= ~(uint32_t)RENDERBUFFER_DIRTY;

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

static void
rb_metal_enable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static void
rb_metal_disable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}
#endif

static render_backend_vtable_t render_backend_vtable_metal = {.construct = rb_metal_construct,
                                                              .destruct = rb_metal_destruct,
                                                              .enumerate_adapters = rb_metal_enumerate_adapters,
                                                              .enumerate_modes = rb_metal_enumerate_modes,
                                                              .target_window_allocate = rb_metal_target_window_allocate,
                                                              .target_deallocate = rb_metal_target_deallocate,
                                                              .pipeline_allocate = rb_metal_pipeline_allocate,
                                                              .pipeline_deallocate = rb_metal_pipeline_deallocate,
                                                              .pipeline_set_color_attachment = rb_metal_pipeline_set_color_attachment,
                                                              .pipeline_set_depth_attachment = rb_metal_pipeline_set_depth_attachment,
                                                              .pipeline_set_color_clear = rb_metal_pipeline_set_color_clear,
                                                              .pipeline_set_depth_clear = rb_metal_pipeline_set_depth_clear,
                                                              .pipeline_flush = rb_metal_pipeline_flush,
                                                              .shader_upload = rb_metal_shader_upload,
                                                              .shader_finalize = rb_metal_shader_finalize};

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
