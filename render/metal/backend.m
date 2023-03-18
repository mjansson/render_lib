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

typedef struct render_pipeline_state_metal_t render_pipeline_state_metal_t;

typedef struct render_backend_metal_t {
	render_backend_t backend;

	id<MTLDevice> device;
	// id<MTLCommandQueue> command_queue;
	// MTLRenderPassDescriptor* render_pass_descriptor;

	//! Pipeline state objects
	hashmap_t* pipeline_state_map;
	render_pipeline_state_metal_t* pipeline_state;
	uint* pipeline_state_free;
	id<MTLBuffer> pipeline_state_storage;
	id<MTLArgumentEncoder> pipeline_state_encoder;

	//! Buffer of buffer objects used for rendering
	mutex_t* buffer_lock;
	uintptr_t* buffer_array;
	render_buffer_t** buffer_lookup;
	id<MTLBuffer> buffer_storage;
	id<MTLArgumentEncoder> buffer_encoder;
	uint* buffer_free;
	uint buffer_count;
	uint buffer_capacity;

	id<MTLDepthStencilState> depth_state;
} render_backend_metal_t;

typedef struct render_target_window_metal_t {
	render_target_t target;
	CAMetalLayer* metal_layer;
} render_target_window_metal_t;

typedef struct render_target_texture_metal_t {
	render_target_t target;
	id<MTLTexture> texture;
	MTLPixelFormat format_metal;
} render_target_texture_metal_t;

typedef struct render_pipeline_metal_t {
	render_pipeline_t pipeline;
	MTLRenderPassDescriptor* descriptor;
	id<MTLCommandQueue> command_queue;
	// render_shader_t* render_compute_shader;
	// id<MTLComputePipelineState> compute_pipeline_state[2];
	// id<MTLIndirectCommandBuffer> indirect_command_buffer;
	// id<MTLBuffer> compute_data;
	render_buffer_index_t* argument_buffer_used;
	render_buffer_index_t* render_buffer_used;
	uint command_capacity;
	uint last_primitive_count;
} render_pipeline_metal_t;

typedef struct render_pipeline_state_metal_t {
	uint index;
	uint ref;
	hash_t hash;
	render_shader_t* shader;
	uintptr_t pipeline_state;
} render_pipeline_state_metal_t;

static bool do_capture;

extern void
rb_metal_do_capture(void);

extern void
rb_metal_do_capture(void) {
	do_capture = true;
}

static void
rb_metal_destruct(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	@autoreleasepool {
		backend_metal->buffer_storage = nil;
		backend_metal->buffer_encoder = nil;
		backend_metal->depth_state = nil;
		backend_metal->pipeline_state_storage = nil;
		backend_metal->pipeline_state_encoder = nil;
		backend_metal->device = nil;
	}

	hashmap_deallocate(backend_metal->pipeline_state_map);
	backend_metal->pipeline_state_map = nullptr;

	array_deallocate(backend_metal->pipeline_state);
	array_deallocate(backend_metal->pipeline_state_free);

	array_deallocate(backend_metal->buffer_free);
	array_deallocate(backend_metal->buffer_array);
	array_deallocate(backend_metal->buffer_lookup);

	mutex_deallocate(backend_metal->buffer_lock);

	log_info(HASH_RENDER, STRING_CONST("Destructed metal render backend"));
}

static bool
rb_metal_construct(render_backend_t* backend) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	backend->shader_type = HASH_SHADER_METAL;

	@autoreleasepool {
		backend_metal->device = MTLCreateSystemDefaultDevice();
		if (!backend_metal->device) {
			log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			          STRING_CONST("Unable to create default system metal device"));
			return false;
		}

		MTLArgumentBuffersTier argument_buffers_tier = [backend_metal->device argumentBuffersSupport];
		if (argument_buffers_tier == MTLArgumentBuffersTier1) {
			rb_metal_destruct(backend);
			log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			          STRING_CONST("Required Metal argument buffers tier 2 not supported by device"));
			return false;
		}

		// Compute shader set_render_pipeline_state available on macOS since Metal 2.1 and on iOS since Metal 2.2
#if FOUNDATION_PLATFORM_IOS

#else

#endif
	}

	backend_metal->pipeline_state_map = hashmap_allocate(127, 32);
	array_reserve(backend_metal->pipeline_state, 1024);

	render_pipeline_state_metal_t default_state = {0};
	array_push(backend_metal->pipeline_state, default_state);

	backend_metal->buffer_lock = mutex_allocate(STRING_CONST("Buffer store"));

	backend_metal->buffer_count = 1;
	backend_metal->buffer_capacity = 32 * 1024;
	array_reserve(backend_metal->buffer_free, backend_metal->buffer_capacity);

	size_t buffer_size = backend_metal->buffer_capacity * sizeof(void*);
	backend_metal->buffer_storage = [backend_metal->device newBufferWithLength:buffer_size
	                                                                   options:MTLResourceStorageModeShared];
	backend_metal->buffer_storage.label = @"Buffer storage";
	array_resize(backend_metal->buffer_array, backend_metal->buffer_capacity);
	array_resize(backend_metal->buffer_lookup, backend_metal->buffer_capacity);

	backend_metal->buffer_array[0] = 0;
	backend_metal->buffer_lookup[0] = 0;

	@autoreleasepool {
		NSMutableArray<MTLArgumentDescriptor*>* argument_descriptor_array = [[NSMutableArray alloc] init];
		for (uint idx = 0; idx < backend_metal->buffer_capacity; ++idx) {
			MTLArgumentDescriptor* argument_descriptor = [MTLArgumentDescriptor argumentDescriptor];
			argument_descriptor.index = idx;
			argument_descriptor.dataType = MTLDataTypePointer;
			argument_descriptor.access = MTLArgumentAccessReadOnly;
			[argument_descriptor_array addObject:argument_descriptor];
		}

		backend_metal->buffer_encoder =
		    [backend_metal->device newArgumentEncoderWithArguments:argument_descriptor_array];
		[backend_metal->buffer_encoder setArgumentBuffer:backend_metal->buffer_storage offset:0];

		argument_descriptor_array = [[NSMutableArray alloc] init];
		for (uint idx = 0; idx < array_capacity(backend_metal->pipeline_state); ++idx) {
			MTLArgumentDescriptor* argument_descriptor = [MTLArgumentDescriptor argumentDescriptor];
			argument_descriptor.index = idx;
			argument_descriptor.dataType = MTLDataTypeRenderPipeline;
			argument_descriptor.access = MTLArgumentAccessReadOnly;
			[argument_descriptor_array addObject:argument_descriptor];
		}

		backend_metal->pipeline_state_encoder =
		    [backend_metal->device newArgumentEncoderWithArguments:argument_descriptor_array];

		buffer_size = backend_metal->pipeline_state_encoder.encodedLength;
		backend_metal->pipeline_state_storage =
		    [backend_metal->device newBufferWithLength:buffer_size options:MTLResourceStorageModeShared];
		backend_metal->pipeline_state_storage.label = @"Pipeline state storage";
		[backend_metal->pipeline_state_encoder setArgumentBuffer:backend_metal->pipeline_state_storage offset:0];

		MTLDepthStencilDescriptor* depth_state_descriptor = [[MTLDepthStencilDescriptor alloc] init];
		depth_state_descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
		depth_state_descriptor.depthWriteEnabled = YES;
		backend_metal->depth_state = [backend_metal->device newDepthStencilStateWithDescriptor:depth_state_descriptor];

		NSString* backend_name = [backend_metal->device name];
		log_infof(HASH_RENDER, STRING_CONST("Metal render backend constructed: %s%s%s%s"), [backend_name UTF8String],
		          [backend_metal->device isHeadless] ? " [headless]" : "",
		          [backend_metal->device isLowPower] ? " [low power]" : "",
		          [backend_metal->device isRemovable] ? " [removable]" : "");
	}

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

	render_target_window_metal_t* target_metal = memory_allocate(HASH_RENDER, sizeof(render_target_window_metal_t), 0,
	                                                             MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
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
		log_error(HASH_RENDER, ERROR_INVALID_VALUE,
		          STRING_CONST("Unable to create Metal render target for window, view has no Metal layer"));
		memory_deallocate(target_metal);
		return 0;
	}

	target_metal->metal_layer.device = backend_metal->device;
	target_metal->metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

	return target;
}

static render_target_t*
rb_metal_target_texture_allocate(render_backend_t* backend, uint width, uint height, render_pixelformat_t format) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	render_target_texture_metal_t* target_metal = memory_allocate(HASH_RENDER, sizeof(render_target_texture_metal_t), 0,
	                                                              MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	render_target_t* target = (render_target_t*)target_metal;
	target->backend = backend;
	target->width = width;
	target->height = height;
	target->type = RENDERTARGET_TEXTURE;
	target->pixelformat = format;
	target->colorspace = COLORSPACE_sRGB;

	MTLPixelFormat format_metal = MTLPixelFormatDepth32Float;

	MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format_metal
	                                                                                      width:width
	                                                                                     height:height
	                                                                                  mipmapped:NO];
	descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
	descriptor.storageMode = MTLStorageModeMemoryless;
	target_metal->texture = [backend_metal->device newTextureWithDescriptor:descriptor];
	target_metal->format_metal = format_metal;

	if (!target_metal->texture) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Unable to create Metal render target for texture"));
		memory_deallocate(target_metal);
		return 0;
	}

	return target;
}

static void
rb_metal_release_metal_library(uintptr_t library) {
	if (!library)
		return;
	@autoreleasepool {
		id<MTLLibrary> metal_library = (__bridge_transfer id<MTLLibrary>)((void*)library);
		metal_library = nil;
		FOUNDATION_UNUSED(metal_library);
	}
}

static void
rb_metal_release_metal_function(uintptr_t function) {
	if (!function)
		return;
	@autoreleasepool {
		id<MTLFunction> metal_function = (__bridge_transfer id<MTLFunction>)((void*)function);
		metal_function = nil;
		FOUNDATION_UNUSED(metal_function);
	}
}

static void
rb_metal_release_metal_buffer(uintptr_t buffer) {
	if (!buffer)
		return;
	@autoreleasepool {
		id<MTLBuffer> metal_buffer = (__bridge_transfer id<MTLBuffer>)((void*)buffer);
		metal_buffer = nil;
		FOUNDATION_UNUSED(metal_buffer);
	}
}

static void
rb_metal_release_metal_argument_encoder(uintptr_t encoder) {
	if (!encoder)
		return;
	@autoreleasepool {
		id<MTLArgumentEncoder> metal_encoder = (__bridge_transfer id<MTLArgumentEncoder>)((void*)encoder);
		metal_encoder = nil;
		FOUNDATION_UNUSED(metal_encoder);
	}
}

static void
rb_metal_target_deallocate(render_backend_t* backend, render_target_t* target) {
	if (!target || (target->backend != backend))
		return;

	if (target->type == RENDERTARGET_WINDOW) {
		// Window target
	} else if (target->type == RENDERTARGET_TEXTURE) {
		// Texture target
		@autoreleasepool {
			render_target_texture_metal_t* target_metal = (render_target_texture_metal_t*)target;
			target_metal->texture = nil;
		}
	}

	memory_deallocate(target);
}

static render_pipeline_t*
rb_metal_pipeline_allocate(render_backend_t* backend, render_indexformat_t index_format, uint capacity) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	render_pipeline_metal_t* pipeline_metal =
	    memory_allocate(HASH_RENDER, sizeof(render_pipeline_metal_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	render_pipeline_t* pipeline = (render_pipeline_t*)pipeline_metal;
	pipeline->backend = backend;
	pipeline->index_format = index_format;

	pipeline_metal->descriptor = [MTLRenderPassDescriptor new];
	if (!pipeline_metal->descriptor) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create Metal render pass descriptor"));
		memory_deallocate(pipeline_metal);
		return 0;
	}

	pipeline_metal->command_queue = [backend_metal->device newCommandQueue];
	if (!pipeline_metal->command_queue) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		          STRING_CONST("Unable to create Metal render pass command queue"));
		memory_deallocate(pipeline_metal);
		return 0;
	}

	pipeline->primitive_buffer =
	    render_buffer_allocate(backend, RENDERUSAGE_RENDER, sizeof(render_primitive_t) * capacity, 0, 0);
	render_buffer_set_label(pipeline->primitive_buffer, STRING_CONST("Pipeline primitive buffer"));

#if 0
	pipeline_metal->render_compute_shader =
	    render_shader_load(backend, uuid_decl(df075392, 1934, 4c89, a45c, 2139d64d9c92));

	id<MTLLibrary> library = (__bridge id<MTLLibrary>)((void*)pipeline_metal->render_compute_shader->backend_data[0]);

	@autoreleasepool {
		NSError* error = 0;
		id<MTLFunction> encoding_kernel = [library newFunctionWithName:@"encoding_kernel_index16"];
		pipeline_metal->compute_pipeline_state[0] =
		    [backend_metal->device newComputePipelineStateWithFunction:encoding_kernel error:&error];

		encoding_kernel = [library newFunctionWithName:@"encoding_kernel_index32"];
		pipeline_metal->compute_pipeline_state[1] =
		    [backend_metal->device newComputePipelineStateWithFunction:encoding_kernel error:&error];

		MTLIndirectCommandBufferDescriptor* indirect_descriptor = [MTLIndirectCommandBufferDescriptor new];
		indirect_descriptor.commandTypes = MTLIndirectCommandTypeDrawIndexed;
		indirect_descriptor.inheritBuffers = NO;
		indirect_descriptor.maxVertexBufferBindCount = 4;
		indirect_descriptor.maxFragmentBufferBindCount = 0;
		indirect_descriptor.inheritPipelineState = NO;
		pipeline_metal->indirect_command_buffer =
		    [backend_metal->device newIndirectCommandBufferWithDescriptor:indirect_descriptor
		                                                  maxCommandCount:capacity
		                                                          options:MTLResourceStorageModeShared];
		pipeline_metal->indirect_command_buffer.label = @"Indirect command buffer";

		id<MTLArgumentEncoder> compute_encoder = [encoding_kernel newArgumentEncoderWithBufferIndex:3];

		pipeline_metal->compute_data = [backend_metal->device newBufferWithLength:compute_encoder.encodedLength
		                                                                  options:MTLResourceStorageModeShared];
		pipeline_metal->compute_data.label = @"Indirect command buffer container";

		[compute_encoder setArgumentBuffer:pipeline_metal->compute_data offset:0];
		[compute_encoder setIndirectCommandBuffer:pipeline_metal->indirect_command_buffer atIndex:0];
	}
#endif
	pipeline_metal->command_capacity = capacity;
	pipeline_metal->last_primitive_count = 0;

	return pipeline;
}

static void
rb_metal_pipeline_deallocate(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;

	@autoreleasepool {
		pipeline_metal->descriptor = nil;
		pipeline_metal->command_queue = nil;
		/*
		pipeline_metal->compute_pipeline_state[0] = nil;
		pipeline_metal->compute_pipeline_state[1] = nil;
		pipeline_metal->indirect_command_buffer = nil;
		pipeline_metal->compute_data = nil;
		*/
	}

	array_deallocate(pipeline_metal->argument_buffer_used);
	array_deallocate(pipeline_metal->render_buffer_used);

	/*
	render_shader_unload(pipeline_metal->render_compute_shader);
	pipeline_metal->render_compute_shader = nullptr;
	*/

	render_buffer_deallocate(pipeline->primitive_buffer);
	memory_deallocate(pipeline);
}

static void
rb_metal_pipeline_set_color_attachment(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                       render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT)
		pipeline->color_attachment[slot] = target;
}

static void
rb_metal_pipeline_set_depth_attachment(render_backend_t* backend, render_pipeline_t* pipeline,
                                       render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (target && (target->type != RENDERTARGET_TEXTURE)) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Invalid render target for depth attachment"));
		return;
	}
	pipeline->depth_attachment = target;
}

static void
rb_metal_pipeline_set_color_clear(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                  render_clear_action_t action, vector_t color) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT) {
		if (action == RENDERCLEAR_CLEAR) {
			pipeline_metal->descriptor.colorAttachments[slot].loadAction = MTLLoadActionClear;
			pipeline_metal->descriptor.colorAttachments[slot].clearColor = MTLClearColorMake(
			    (double)vector_x(color), (double)vector_y(color), (double)vector_z(color), (double)vector_w(color));
		} else if (action == RENDERCLEAR_PRESERVE) {
			pipeline_metal->descriptor.colorAttachments[slot].loadAction = MTLLoadActionLoad;
		} else {
			pipeline_metal->descriptor.colorAttachments[slot].loadAction = MTLLoadActionDontCare;
		}
		pipeline_metal->descriptor.colorAttachments[slot].storeAction = MTLStoreActionStore;
	}
}

static void
rb_metal_pipeline_set_depth_clear(render_backend_t* backend, render_pipeline_t* pipeline, render_clear_action_t action,
                                  vector_t color) {
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
	pipeline_metal->descriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
}

static inline id<MTLBuffer>
rb_metal_buffer_from_index(render_backend_metal_t* backend, uint index) {
#if BUILD_DEBUG
	render_buffer_t* buffer = backend->buffer_lookup[index];
	if (index && (buffer->render_index != index)) {
		log_errorf(0, ERROR_INVALID_VALUE, STRING_CONST("Invalid buffer render index for buffer %u"), index);
		return 0;
	}
#endif
	return (__bridge id<MTLBuffer>)((void*)backend->buffer_array[index]);
}

static inline id<MTLRenderPipelineState>
rb_metal_pipeline_state_from_index(render_backend_metal_t* backend, uint index) {
	render_pipeline_state_metal_t* pipeline_state = backend->pipeline_state + index;
	return (__bridge id<MTLRenderPipelineState>)((void*)pipeline_state->pipeline_state);
}

static void
rb_metal_pipeline_use_argument_buffer(render_backend_t* backend, render_pipeline_t* pipeline,
                                      render_buffer_index_t buffer) {
	FOUNDATION_UNUSED(backend, pipeline, buffer);
	/*
	// This must be thread safe
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;
	array_push(pipeline_metal->argument_buffer_used, buffer);
	*/
}

static void
rb_metal_pipeline_use_render_buffer(render_backend_t* backend, render_pipeline_t* pipeline,
                                    render_buffer_index_t buffer) {
	FOUNDATION_UNUSED(backend);
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;
	array_push(pipeline_metal->render_buffer_used, buffer);
}

static void
rb_metal_pipeline_build(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend, pipeline);
}

static void
rb_metal_pipeline_flush(render_backend_t* backend, render_pipeline_t* pipeline) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	render_pipeline_metal_t* pipeline_metal = (render_pipeline_metal_t*)pipeline;

	id<MTLBuffer> primitive_buffer = (__bridge id<MTLBuffer>)((void*)pipeline->primitive_buffer->backend_data[0]);
	id<MTLBuffer> descriptor_buffer[4];

#if BUILD_DEBUG
	mutex_lock(backend_metal->buffer_lock);
	for (uint ibuf = 1; ibuf < backend_metal->buffer_count; ++ibuf) {
		if (backend_metal->buffer_lookup[ibuf] && (backend_metal->buffer_lookup[ibuf]->render_index != ibuf)) {
			log_errorf(0, ERROR_INVALID_VALUE, STRING_CONST("Buffer render index mismatch when flushing: %u %u"), ibuf,
			           backend_metal->buffer_lookup[ibuf]->render_index);
			exception_raise_abort();
		}
	}
	mutex_unlock(backend_metal->buffer_lock);
#endif

	@autoreleasepool {
		id<CAMetalDrawable> current_drawable = 0;

		render_target_t* target = pipeline->color_attachment[0];
		if (target && (target->type == RENDERTARGET_WINDOW)) {
			render_target_window_metal_t* target_window = (render_target_window_metal_t*)pipeline->color_attachment[0];
			current_drawable = [target_window->metal_layer nextDrawable];
		}

		MTLRenderPassDescriptor* desc = pipeline_metal->descriptor;
		desc.colorAttachments[0].texture = current_drawable.texture;
		if (pipeline->depth_attachment)
			desc.depthAttachment.texture = ((render_target_texture_metal_t*)pipeline->depth_attachment)->texture;

		MTLCaptureManager* capture_manager = nil;
		if (do_capture) {
			do_capture = false;
			capture_manager = [MTLCaptureManager sharedCaptureManager];
			MTLCaptureDescriptor* capture_descriptor = [[MTLCaptureDescriptor alloc] init];
			capture_descriptor.captureObject = backend_metal->device;
			capture_descriptor.outputURL = [NSURL fileURLWithPath:@"render.gputrace"];
			capture_descriptor.destination = MTLCaptureDestinationGPUTraceDocument;

			fs_remove_directory(STRING_CONST("render.gputrace"));

			dispatch_sync(dispatch_get_main_queue(), ^{
			  NSError* error = 0;
			  if (![capture_manager startCaptureWithDescriptor:capture_descriptor error:&error]) {
				  NSLog(@"Failed to start capture, error %@", error);
			  }
			});
		}

		id<MTLCommandBuffer> command_buffer = [pipeline_metal->command_queue commandBuffer];
		command_buffer.label = @"Command buffer";
#if 0
		// Encode command to reset the indirect command buffer
		{
			id<MTLBlitCommandEncoder> reset_blit_encoder = [command_buffer blitCommandEncoder];
			[reset_blit_encoder resetCommandsInBuffer:pipeline_metal->indirect_command_buffer
			                                withRange:NSMakeRange(0, pipeline_metal->command_capacity)];
			[reset_blit_encoder endEncoding];
		}

		// Encode commands using compute kernel
		{
			id<MTLComputeCommandEncoder> compute_encoder = [command_buffer computeCommandEncoder];
			compute_encoder.label = @"Compute encoder";

			[compute_encoder setComputePipelineState:pipeline_metal->compute_pipeline_state[pipeline->index_format]];

			[compute_encoder setBuffer:primitive_buffer offset:0 atIndex:0];
			[compute_encoder setBuffer:backend_metal->buffer_storage offset:0 atIndex:1];
			[compute_encoder setBuffer:backend_metal->pipeline_state_storage offset:0 atIndex:2];
			[compute_encoder setBuffer:pipeline_metal->compute_data offset:0 atIndex:3];

			[compute_encoder useResource:pipeline_metal->indirect_command_buffer usage:MTLResourceUsageWrite];

			for (size_t ibuf = 0, bcount = array_count(pipeline_metal->argument_buffer_used); ibuf < bcount; ++ibuf) {
				id<MTLBuffer> buffer =
				    rb_metal_buffer_from_index(backend_metal, pipeline_metal->argument_buffer_used[ibuf]);
				[compute_encoder useResource:buffer usage:MTLResourceUsageRead];
			}

			NSUInteger thread_execution_width =
			    pipeline_metal->compute_pipeline_state[pipeline->index_format].threadExecutionWidth;
			[compute_encoder dispatchThreads:MTLSizeMake(pipeline->primitive_buffer->used, 1, 1)
			           threadsPerThreadgroup:MTLSizeMake(thread_execution_width, 1, 1)];

			[compute_encoder endEncoding];
		}

		// Encode command to optimize the indirect command buffer after encoding
		{
			id<MTLBlitCommandEncoder> optimize_blit_encoder = [command_buffer blitCommandEncoder];
			[optimize_blit_encoder optimizeIndirectCommandBuffer:pipeline_metal->indirect_command_buffer
			                                           withRange:NSMakeRange(0, pipeline->primitive_buffer->used)];
			[optimize_blit_encoder endEncoding];
		}

		pipeline_metal->last_primitive_count = (uint)pipeline->primitive_buffer->used;

		id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor:desc];

		[render_encoder setCullMode:MTLCullModeBack];
		[render_encoder setFrontFacingWinding:MTLWindingCounterClockwise];
		if (pipeline->depth_attachment)
			[render_encoder setDepthStencilState:backend_metal->depth_state];

		for (size_t ibuf = 0, bcount = array_count(pipeline_metal->render_buffer_used); ibuf < bcount; ++ibuf) {
			id<MTLBuffer> buffer = rb_metal_buffer_from_index(backend_metal, pipeline_metal->render_buffer_used[ibuf]);
			[render_encoder useResource:buffer usage:MTLResourceUsageRead];
		}

		// Compute pipeline
		[render_encoder executeCommandsInBuffer:pipeline_metal->indirect_command_buffer
		                              withRange:NSMakeRange(0, pipeline->primitive_buffer->used)];
		[render_encoder endEncoding];

#else
		FOUNDATION_UNUSED(primitive_buffer);
		id<MTLRenderCommandEncoder> render_encoder = [command_buffer renderCommandEncoderWithDescriptor:desc];

		[render_encoder setCullMode:MTLCullModeBack];
		[render_encoder setFrontFacingWinding:MTLWindingCounterClockwise];
		if (pipeline->depth_attachment)
			[render_encoder setDepthStencilState:backend_metal->depth_state];

		for (size_t ibuf = 0, bcount = array_count(pipeline_metal->render_buffer_used); ibuf < bcount; ++ibuf) {
			id<MTLBuffer> buffer = rb_metal_buffer_from_index(backend_metal, pipeline_metal->render_buffer_used[ibuf]);
			[render_encoder useResource:buffer usage:MTLResourceUsageRead];
		}

		render_buffer_lock(pipeline->primitive_buffer, RENDERBUFFER_LOCK_READ);
		render_primitive_t* primitives = pipeline->primitive_buffer->access;
		render_pipeline_state_t current_state = 0;
		render_buffer_index_t current_argument = 0;
		render_buffer_t* argument_buffer = 0;
		size_t primitive_count = pipeline->primitive_buffer->used;
		for (size_t iprim = 0; iprim < primitive_count; ++iprim) {
			render_primitive_t* primitive = primitives + iprim;
			if (primitive->pipeline_state != current_state) {
				current_state = primitive->pipeline_state;
				id<MTLRenderPipelineState> pipeline_state =
				    rb_metal_pipeline_state_from_index(backend_metal, current_state);
				[render_encoder setRenderPipelineState:pipeline_state];
			}
			if (primitive->argument_buffer != current_argument) {
				if (argument_buffer)
					render_buffer_unlock(argument_buffer);
				current_argument = primitive->argument_buffer;
				argument_buffer = backend_metal->buffer_lookup[current_argument];
				FOUNDATION_ASSERT(argument_buffer);
				render_buffer_lock(argument_buffer, RENDERBUFFER_LOCK_READ);
			}

			descriptor_buffer[0] = rb_metal_buffer_from_index(backend_metal, primitive->descriptor[0]);
			descriptor_buffer[1] = rb_metal_buffer_from_index(backend_metal, primitive->descriptor[1]);
			descriptor_buffer[2] = rb_metal_buffer_from_index(backend_metal, primitive->descriptor[2]);
			descriptor_buffer[3] = rb_metal_buffer_from_index(backend_metal, primitive->descriptor[3]);

			[render_encoder setVertexBuffer:descriptor_buffer[0] offset:0 atIndex:0];
			[render_encoder setVertexBuffer:descriptor_buffer[1] offset:0 atIndex:1];
			[render_encoder setVertexBuffer:descriptor_buffer[2] offset:0 atIndex:2];
			[render_encoder setVertexBuffer:descriptor_buffer[3] offset:0 atIndex:3];

			render_argument_t* argument =
			    (render_argument_t*)pointer_offset(argument_buffer->access, primitive->argument_offset);
			MTLIndexType index_type =
			    (pipeline->index_format == RENDER_INDEXFORMAT_UINT16) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
			id<MTLBuffer> index_buffer = rb_metal_buffer_from_index(backend_metal, primitive->index_buffer);
			[render_encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
			                           indexCount:argument->index_count
			                            indexType:index_type
			                          indexBuffer:index_buffer
			                    indexBufferOffset:argument->index_offset
			                        instanceCount:argument->instance_count
			                           baseVertex:argument->vertex_base
			                         baseInstance:argument->instance_base];
		}
		if (argument_buffer)
			render_buffer_unlock(argument_buffer);
		render_buffer_unlock(pipeline->primitive_buffer);

		[render_encoder endEncoding];

#endif
		[command_buffer presentDrawable:current_drawable afterMinimumDuration:0.008333];
		[command_buffer commit];

		if (capture_manager) {
			log_infof(HASH_RENDER, STRING_CONST("Rendered %u primitives"), (uint)pipeline->primitive_buffer->used);
			[command_buffer waitUntilCompleted];
			dispatch_sync(dispatch_get_main_queue(), ^{
			  [capture_manager stopCapture];
			});
			string_const_t current_path = environment_current_working_directory();
			log_infof(HASH_RENDER, STRING_CONST("Capture frame complete: %.*s"), STRING_FORMAT(current_path));
			capture_manager = nil;
		}
	}

	array_clear(pipeline_metal->argument_buffer_used);
	array_clear(pipeline_metal->render_buffer_used);
}

static render_pipeline_state_t
rb_metal_pipeline_state_allocate(render_backend_t* backend, render_pipeline_t* pipeline, render_shader_t* shader) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	render_pipeline_state_metal_t state_metal = {0};
	state_metal.shader = shader;

	hash_t state_hash = hash(&state_metal.shader, sizeof(state_metal.shader));
	void* stored_state = hashmap_lookup(backend_metal->pipeline_state_map, state_hash);
	render_pipeline_state_t state_previous_index = (render_pipeline_state_t)((uintptr_t)stored_state);
	if (state_previous_index) {
		render_pipeline_state_metal_t* state_previous = backend_metal->pipeline_state + state_previous_index;
		++state_previous->ref;
		return state_previous_index;
	}

	state_metal.ref = 1;
	state_metal.hash = state_hash;

	@autoreleasepool {
		NSError* error = nil;
		MTLRenderPipelineDescriptor* pipeline_state_descriptor = [[MTLRenderPipelineDescriptor alloc] init];
		pipeline_state_descriptor.sampleCount = 1;
		pipeline_state_descriptor.vertexFunction = (__bridge id<MTLFunction>)((void*)shader->backend_data[1]);
		pipeline_state_descriptor.fragmentFunction = (__bridge id<MTLFunction>)((void*)shader->backend_data[2]);
		pipeline_state_descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
		if (pipeline->depth_attachment) {
			render_target_texture_metal_t* target_metal = (render_target_texture_metal_t*)pipeline->depth_attachment;
			pipeline_state_descriptor.depthAttachmentPixelFormat = target_metal->format_metal;
		}
		// Needed for this pipeline state to be used in indirect command buffers
		pipeline_state_descriptor.supportIndirectCommandBuffers = TRUE;

		id<MTLRenderPipelineState> pipeline_state =
		    [backend_metal->device newRenderPipelineStateWithDescriptor:pipeline_state_descriptor error:&error];
		state_metal.pipeline_state = (uintptr_t)((__bridge_retained void*)pipeline_state);
	}

	uint free_count = array_count(backend_metal->pipeline_state_free);
	if (free_count) {
		state_metal.index = backend_metal->pipeline_state_free[free_count - 1];
		array_pop(backend_metal->pipeline_state_free);
		backend_metal->pipeline_state[state_metal.index] = state_metal;
	} else {
		state_metal.index = array_count(backend_metal->pipeline_state);
		array_push(backend_metal->pipeline_state, state_metal);
	}
	hashmap_insert(backend_metal->pipeline_state_map, state_metal.hash, (void*)((uintptr_t)state_metal.index));

	// Store the state in the array of states
	{
		id<MTLRenderPipelineState> pipeline_state =
		    (__bridge id<MTLRenderPipelineState>)((void*)state_metal.pipeline_state);
		[backend_metal->pipeline_state_encoder setRenderPipelineState:pipeline_state atIndex:state_metal.index];
	}

	return state_metal.index;
}

static void
rb_metal_pipeline_state_deallocate(render_backend_t* backend, render_pipeline_state_t state) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;
	if (!state || (state >= array_count(backend_metal->pipeline_state)))
		return;

	render_pipeline_state_metal_t* state_metal = backend_metal->pipeline_state + state;
	if (!state_metal->ref)
		return;

	if (--state_metal->ref)
		return;

	array_push(backend_metal->pipeline_state_free, state_metal->index);
}

static bool
rb_metal_shader_upload(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	if (shader->backend_data[0]) {
		@autoreleasepool {
			rb_metal_release_metal_library(shader->backend_data[0]);
			shader->backend_data[0] = 0;
		}
	}

	@autoreleasepool {
		NSError* error = 0;
		dispatch_data_t data = dispatch_data_create(buffer, size, 0, DISPATCH_DATA_DESTRUCTOR_DEFAULT);

		id<MTLLibrary> library = [backend_metal->device newLibraryWithData:data error:&error];
		if (!library) {
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to upload Metal shader library"));
			return false;
		}
		shader->backend_data[0] = (uintptr_t)((__bridge_retained void*)library);

		id<MTLFunction> vertex_function = [library newFunctionWithName:@"vertex_shader"];
		if (vertex_function)
			shader->backend_data[1] = (uintptr_t)((__bridge_retained void*)vertex_function);

		id<MTLFunction> pixel_function = [library newFunctionWithName:@"pixel_shader"];
		if (pixel_function)
			shader->backend_data[2] = (uintptr_t)((__bridge_retained void*)pixel_function);
	}

	return true;
}

static void
rb_metal_shader_finalize(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend);
	rb_metal_release_metal_library(shader->backend_data[0]);
	shader->backend_data[0] = 0;

	rb_metal_release_metal_function(shader->backend_data[1]);
	shader->backend_data[1] = 0;

	rb_metal_release_metal_function(shader->backend_data[2]);
	shader->backend_data[2] = 0;

	rb_metal_release_metal_function(shader->backend_data[3]);
	shader->backend_data[3] = 0;
}

static void
rb_metal_buffer_allocate(render_backend_t* backend, render_buffer_t* buffer, size_t buffer_size, const void* data,
                         size_t data_size) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	if (buffer_size % 16)
		buffer_size += 16 - (buffer_size % 16);

	@autoreleasepool {
		id<MTLBuffer> metal_buffer;
		if (buffer->usage & RENDERUSAGE_GPUONLY) {
			buffer->usage &= ~(uint)RENDERUSAGE_CPUONLY;
			metal_buffer = [backend_metal->device newBufferWithLength:buffer_size
			                                                  options:MTLResourceStorageModePrivate];
			if (!metal_buffer) {
				log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Unable to allocate GPU only Metal buffer"));
			} else {
				buffer->allocated = buffer_size;
			}
			if (data_size) {
				log_error(HASH_RENDER, ERROR_INVALID_VALUE,
				          STRING_CONST("Unable to allocate GPU only Metal buffer with given data"));
			}
			buffer->used = buffer_size;
		} else if (buffer->usage & RENDERUSAGE_CPUONLY) {
			buffer->usage &= ~(uint)RENDERUSAGE_GPUONLY;
			buffer->store = memory_allocate(HASH_RENDER, buffer_size, 0, MEMORY_PERSISTENT);
			buffer->allocated = buffer_size;
			if (data_size && buffer->store) {
				memcpy(buffer->store, data, data_size);
				buffer->used = data_size;
			}
			buffer->usage &= ~(uint8_t)RENDERUSAGE_RENDER;
		} else {
#if FOUNDATION_PLATFORM_MACOS
			uint storage_mode = MTLResourceStorageModeManaged;
#else
			uint storage_mode = MTLResourceStorageModeShared;
#endif
			metal_buffer = [backend_metal->device newBufferWithLength:buffer_size options:storage_mode];
			if (!metal_buffer) {
				log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Unable to allocate GPU only Metal buffer"));
			} else {
				buffer->store = [metal_buffer contents];
				buffer->allocated = buffer_size;

				if (data_size > buffer_size)
					data_size = buffer_size;
				if (data_size) {
					memcpy(buffer->store, data, data_size);
					buffer->used = data_size;
#if FOUNDATION_PLATFORM_MACOS
					NSRange range;
					range.location = 0;
					range.length = data_size;
					[metal_buffer didModifyRange:range];
#endif
				}
			}
		}

		if (metal_buffer) {
			uintptr_t buffer_handle = (uintptr_t)((__bridge_retained void*)metal_buffer);
			if ((buffer->usage & RENDERUSAGE_RENDER) && (!buffer->render_index)) {
				mutex_lock(backend_metal->buffer_lock);

				uint render_index = 0;
				uint buffer_free_count = array_count(backend_metal->buffer_free);
				if (buffer_free_count) {
					render_index = backend_metal->buffer_free[buffer_free_count - 1];
					array_pop(backend_metal->buffer_free);
				} else {
					if (backend_metal->buffer_count >= backend_metal->buffer_capacity) {
						// TODO: Reallocate storage
						log_error(HASH_RENDER, ERROR_OUT_OF_MEMORY,
						          STRING_CONST("Unable to allocate more render buffers"));
					} else {
						render_index = backend_metal->buffer_count++;
					}
				}
				buffer->render_index = render_index;

				if (render_index) {
					// Store the buffer in the array of buffers
					[backend_metal->buffer_encoder setBuffer:metal_buffer offset:0 atIndex:render_index];
					backend_metal->buffer_array[render_index] = buffer_handle;
					backend_metal->buffer_lookup[render_index] = buffer;
				} else {
					log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Failed to get buffer render index"));
					exception_raise_abort();
				}

#if BUILD_DEBUG
				for (uint ibuf = 1; ibuf < backend_metal->buffer_count; ++ibuf) {
					if (backend_metal->buffer_lookup[ibuf] &&
					    (backend_metal->buffer_lookup[ibuf]->render_index != ibuf)) {
						log_errorf(0, ERROR_INVALID_VALUE,
						           STRING_CONST("Buffer render index mismatch after creating buffer %u: %u %u"),
						           render_index, ibuf, backend_metal->buffer_lookup[ibuf]->render_index);
						exception_raise_abort();
					}
				}
#endif

				mutex_unlock(backend_metal->buffer_lock);
			}
			buffer->backend_data[0] = buffer_handle;
		}
	}
}

static void
rb_metal_buffer_deallocate(render_backend_t* backend, render_buffer_t* buffer, bool cpu, bool gpu) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	if (cpu && (buffer->usage & RENDERUSAGE_CPUONLY) && buffer->store) {
		memory_deallocate(buffer->store);
		buffer->store = nullptr;
	}

	if (gpu) {
		if (buffer->backend_data[0]) {
			rb_metal_release_metal_buffer(buffer->backend_data[0]);
			buffer->backend_data[0] = 0;
		}

		if (buffer->backend_data[1]) {
			rb_metal_release_metal_argument_encoder(buffer->backend_data[1]);
			buffer->backend_data[1] = 0;
		}

		uint render_index = buffer->render_index;
		buffer->render_index = 0;

		if (render_index) {
			backend_metal->buffer_lookup[render_index] = 0;

			mutex_lock(backend_metal->buffer_lock);
			array_push(backend_metal->buffer_free, render_index);
			mutex_unlock(backend_metal->buffer_lock);
		}
	}
}

static void
rb_metal_buffer_upload(render_backend_t* backend, render_buffer_t* buffer, size_t offset, size_t size) {
	FOUNDATION_UNUSED(backend);
	if ((buffer->usage & RENDERUSAGE_CPUONLY) || (buffer->usage & RENDERUSAGE_GPUONLY) || !buffer->backend_data[0])
		return;

#if FOUNDATION_PLATFORM_MACOS
	if (offset > buffer->allocated)
		offset = buffer->allocated;
	if (!size)
		size = buffer->allocated;
	if ((offset + size) > buffer->allocated)
		size = buffer->allocated - offset;
	id<MTLBuffer> metal_buffer = (__bridge id<MTLBuffer>)((void*)buffer->backend_data[0]);
	NSRange range;
	range.location = offset;
	range.length = size;
	[metal_buffer didModifyRange:range];
#endif
}

static void
rb_metal_buffer_set_label(render_backend_t* backend, render_buffer_t* buffer, const char* name, size_t length) {
	FOUNDATION_UNUSED(backend, length);
	id<MTLBuffer> metal_buffer = (__bridge id<MTLBuffer>)((void*)buffer->backend_data[0]);
	[metal_buffer setLabel:[NSString stringWithUTF8String:name]];
}

static void
rb_metal_buffer_data_declare(render_backend_t* backend, render_buffer_t* buffer, size_t instance_count,
                             const render_buffer_data_t* data, size_t data_count) {
	render_backend_metal_t* backend_metal = (render_backend_metal_t*)backend;

	@autoreleasepool {
		NSMutableArray<MTLArgumentDescriptor*>* argument_descriptor_array = [[NSMutableArray alloc] init];
		for (uint iarg = 0; iarg < data_count; ++iarg) {
			MTLArgumentDescriptor* argument_descriptor = [MTLArgumentDescriptor argumentDescriptor];
			argument_descriptor.index = iarg;
			if (data[iarg].data_type == RENDERDATA_POINTER) {
				argument_descriptor.dataType = MTLDataTypePointer;
			} else if (data[iarg].data_type == RENDERDATA_FLOAT4) {
				argument_descriptor.dataType = MTLDataTypeFloat4;
			} else if (data[iarg].data_type == RENDERDATA_MATRIX4X4) {
				argument_descriptor.dataType = MTLDataTypeFloat4x4;
			} else {
				log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Invalid buffer structured data type"));
				return;
			}
			argument_descriptor.arrayLength = data[iarg].array_count;
			argument_descriptor.access = MTLArgumentAccessReadOnly;
			[argument_descriptor_array addObject:argument_descriptor];
		}

		id<MTLArgumentEncoder> argument_encoder =
		    [backend_metal->device newArgumentEncoderWithArguments:argument_descriptor_array];

		size_t total_size = [argument_encoder encodedLength] * instance_count;
		if (buffer->allocated < total_size) {
			rb_metal_buffer_deallocate(backend, buffer, true, true);
			rb_metal_buffer_allocate(backend, buffer, total_size, 0, 0);
		}

		[argument_encoder setArgumentBuffer:(__bridge id<MTLBuffer>)((void*)buffer->backend_data[0]) offset:0];

		if (buffer->backend_data[1])
			rb_metal_release_metal_argument_encoder(buffer->backend_data[1]);
		buffer->backend_data[1] = (uintptr_t)((__bridge_retained void*)argument_encoder);
	}
}

static void
rb_metal_buffer_data_encode_buffer(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                   render_buffer_t* source, uint offset) {
	FOUNDATION_UNUSED(backend);
	if (!buffer->backend_data[1]) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE,
		          STRING_CONST("Unable to encode buffer structured data without previous data layout declaration"));
		exception_raise_abort();
	}

	id<MTLArgumentEncoder> encoder = (__bridge id<MTLArgumentEncoder>)((void*)buffer->backend_data[1]);
	id<MTLBuffer> metal_buffer = (__bridge id<MTLBuffer>)((void*)buffer->backend_data[0]);
	uint instance_size = (uint)[encoder encodedLength];
	uint instance_offset = instance * instance_size;
	[encoder setArgumentBuffer:metal_buffer offset:instance_offset];

	metal_buffer = (__bridge id<MTLBuffer>)((void*)source->backend_data[0]);
	[encoder setBuffer:metal_buffer offset:offset atIndex:index];
}

static void
rb_metal_buffer_data_encode_matrix(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                   const matrix_t* matrix) {
	FOUNDATION_UNUSED(backend);
	if (!buffer->backend_data[1]) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE,
		          STRING_CONST("Unable to encode buffer structured data without previous data layout declaration"));
		return;
	}

	id<MTLArgumentEncoder> encoder = (__bridge id<MTLArgumentEncoder>)((void*)buffer->backend_data[1]);
	id<MTLBuffer> metal_buffer = (__bridge id<MTLBuffer>)((void*)buffer->backend_data[0]);
	uint instance_size = (uint)[encoder encodedLength];
	uint instance_offset = instance * instance_size;
	[encoder setArgumentBuffer:metal_buffer offset:instance_offset];

	void* buffer_data = [encoder constantDataAtIndex:index];
	if (buffer_data) {
		matrix_t transposed = matrix_transpose(*matrix);
		memcpy(buffer_data, &transposed, sizeof(matrix_t));
	}
}

static void
rb_metal_buffer_data_encode_constant(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                     const void* data, uint size) {
	FOUNDATION_UNUSED(backend);
	if (!buffer->backend_data[1]) {
		log_error(HASH_RENDER, ERROR_INVALID_VALUE,
		          STRING_CONST("Unable to encode buffer structured data without previous data layout declaration"));
		return;
	}

	id<MTLArgumentEncoder> encoder = (__bridge id<MTLArgumentEncoder>)((void*)buffer->backend_data[1]);
	id<MTLBuffer> metal_buffer = (__bridge id<MTLBuffer>)((void*)buffer->backend_data[0]);
	uint instance_size = (uint)[encoder encodedLength];
	uint instance_offset = instance * instance_size;
	[encoder setArgumentBuffer:metal_buffer offset:instance_offset];

	void* buffer_data = [encoder constantDataAtIndex:index];
	if (buffer_data) {
		memcpy(buffer_data, data, size);
	}
}

static render_backend_vtable_t render_backend_vtable_metal = {
    .construct = rb_metal_construct,
    .destruct = rb_metal_destruct,
    .enumerate_adapters = rb_metal_enumerate_adapters,
    .enumerate_modes = rb_metal_enumerate_modes,
    .target_window_allocate = rb_metal_target_window_allocate,
    .target_texture_allocate = rb_metal_target_texture_allocate,
    .target_deallocate = rb_metal_target_deallocate,
    .pipeline_allocate = rb_metal_pipeline_allocate,
    .pipeline_deallocate = rb_metal_pipeline_deallocate,
    .pipeline_set_color_attachment = rb_metal_pipeline_set_color_attachment,
    .pipeline_set_depth_attachment = rb_metal_pipeline_set_depth_attachment,
    .pipeline_set_color_clear = rb_metal_pipeline_set_color_clear,
    .pipeline_set_depth_clear = rb_metal_pipeline_set_depth_clear,
    .pipeline_build = rb_metal_pipeline_build,
    .pipeline_flush = rb_metal_pipeline_flush,
    .pipeline_use_argument_buffer = rb_metal_pipeline_use_argument_buffer,
    .pipeline_use_render_buffer = rb_metal_pipeline_use_render_buffer,
    .pipeline_state_allocate = rb_metal_pipeline_state_allocate,
    .pipeline_state_deallocate = rb_metal_pipeline_state_deallocate,
    .shader_upload = rb_metal_shader_upload,
    .shader_finalize = rb_metal_shader_finalize,
    .buffer_allocate = rb_metal_buffer_allocate,
    .buffer_deallocate = rb_metal_buffer_deallocate,
    .buffer_upload = rb_metal_buffer_upload,
    .buffer_set_label = rb_metal_buffer_set_label,
    .buffer_data_declare = rb_metal_buffer_data_declare,
    .buffer_data_encode_buffer = rb_metal_buffer_data_encode_buffer,
    .buffer_data_encode_matrix = rb_metal_buffer_data_encode_matrix,
    .buffer_data_encode_constant = rb_metal_buffer_data_encode_constant};

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
