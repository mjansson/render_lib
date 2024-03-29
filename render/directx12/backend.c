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

#include <render/directx12/backend.h>

#if FOUNDATION_PLATFORM_WINDOWS

static bool
rb_dx12_construct(render_backend_t* backend) {
	backend->shader_type = HASH_SHADER_DIRECTX12;
	log_debug(HASH_RENDER, STRING_CONST("Constructed DirectX 12 render backend"));
	return true;
}

static void
rb_dx12_destruct(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
	log_debug(HASH_RENDER, STRING_CONST("Destructed DirectX 12 render backend"));
}

static size_t
rb_dx12_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	if (capacity)
		store[0] = (unsigned int)WINDOW_ADAPTER_DEFAULT;
	return 1;
}

static size_t
rb_dx12_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(adapter);
	if (capacity) {
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, 60};
		store[0] = mode;
	}
	return 1;
}

static render_target_t*
rb_dx12_target_window_allocate(render_backend_t* backend, window_t* window, uint tag) {
	FOUNDATION_UNUSED(tag);
	render_target_t* target =
	    memory_allocate(HASH_RENDER, sizeof(render_target_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	target->backend = backend;
	target->width = window_width(window);
	target->height = window_height(window);
	target->type = RENDERTARGET_WINDOW;
	target->pixelformat = PIXELFORMAT_R8G8B8A8;
	target->colorspace = COLORSPACE_sRGB;
	return target;
}

static render_target_t*
rb_dx12_target_texture_allocate(render_backend_t* backend, uint width, uint height, render_pixelformat_t format) {
	render_target_t* target =
	    memory_allocate(HASH_RENDER, sizeof(render_target_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	target->backend = backend;
	target->width = width;
	target->height = height;
	target->type = RENDERTARGET_TEXTURE;
	target->pixelformat = format;
	target->colorspace = COLORSPACE_sRGB;
	return target;
}

static void
rb_dx12_target_deallocate(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	memory_deallocate(target);
}

static render_pipeline_t*
rb_dx12_pipeline_allocate(render_backend_t* backend, render_indexformat_t index_format, uint capacity) {
	render_pipeline_t* pipeline =
	    memory_allocate(HASH_RENDER, sizeof(render_pipeline_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	pipeline->backend = backend;
	pipeline->primitive_buffer =
	    render_buffer_allocate(backend, RENDERUSAGE_RENDER, sizeof(render_primitive_t) * capacity, 0, 0);
	pipeline->index_format = index_format;
	return pipeline;
}

static void
rb_dx12_pipeline_deallocate(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend);
	if (pipeline)
		render_buffer_deallocate(pipeline->primitive_buffer);
	memory_deallocate(pipeline);
}

static void
rb_dx12_pipeline_set_color_attachment(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                      render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (slot < RENDER_TARGET_COLOR_ATTACHMENT_COUNT)
		pipeline->color_attachment[slot] = target;
}

static void
rb_dx12_pipeline_set_depth_attachment(render_backend_t* backend, render_pipeline_t* pipeline, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	pipeline->depth_attachment = target;
}

static void
rb_dx12_pipeline_set_color_clear(render_backend_t* backend, render_pipeline_t* pipeline, uint slot,
                                 render_clear_action_t action, vector_t color) {
	FOUNDATION_UNUSED(backend, pipeline, slot, action, color);
}

static void
rb_dx12_pipeline_set_depth_clear(render_backend_t* backend, render_pipeline_t* pipeline, render_clear_action_t action,
                                 vector_t color) {
	FOUNDATION_UNUSED(backend, pipeline, action, color);
}

static void
rb_dx12_pipeline_build(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend, pipeline);
}

static void
rb_dx12_pipeline_flush(render_backend_t* backend, render_pipeline_t* pipeline) {
	FOUNDATION_UNUSED(backend, pipeline);
}

static void
rb_dx12_pipeline_use_argument_buffer(render_backend_t* backend, render_pipeline_t* pipeline,
                                     render_buffer_index_t buffer) {
	FOUNDATION_UNUSED(backend, pipeline, buffer);
}

static void
rb_dx12_pipeline_use_render_buffer(render_backend_t* backend, render_pipeline_t* pipeline,
                                   render_buffer_index_t buffer) {
	FOUNDATION_UNUSED(backend, pipeline, buffer);
}

static render_pipeline_state_t
rb_dx12_pipeline_state_allocate(render_backend_t* backend, render_pipeline_t* pipeline, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend, pipeline, shader);
	return 0;
}

static void
rb_dx12_pipeline_state_deallocate(render_backend_t* backend, render_pipeline_state_t state) {
	FOUNDATION_UNUSED(backend, state);
}

static bool
rb_dx12_shader_upload(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend, shader, buffer, size);
	return true;
}

static void
rb_dx12_shader_finalize(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend, shader);
}

static void
rb_dx12_buffer_allocate(render_backend_t* backend, render_buffer_t* buffer, size_t buffer_size, const void* data,
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
rb_dx12_buffer_deallocate(render_backend_t* backend, render_buffer_t* buffer, bool cpu, bool gpu) {
	FOUNDATION_UNUSED(backend, gpu);
	if (cpu && buffer->store) {
		memory_deallocate(buffer->store);
		buffer->store = nullptr;
	}
}

static void
rb_dx12_buffer_upload(render_backend_t* backend, render_buffer_t* buffer, size_t offset, size_t size) {
	FOUNDATION_UNUSED(backend, buffer, offset, size);
}

static void
rb_dx12_buffer_data_declare(render_backend_t* backend, render_buffer_t* buffer, size_t instance_count,
                            const render_buffer_data_t* data, size_t data_count) {
	FOUNDATION_UNUSED(backend, buffer, data, data_count, instance_count);
}

static void
rb_dx12_buffer_data_encode_buffer(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                  render_buffer_t* source, uint offset) {
	FOUNDATION_UNUSED(backend, buffer, instance, index, source, offset);
}

static void
rb_dx12_buffer_data_encode_constant(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                    const void* data, uint size) {
	FOUNDATION_UNUSED(backend, buffer, instance, index, data, size);
}

static void
rb_dx12_buffer_data_encode_matrix(render_backend_t* backend, render_buffer_t* buffer, uint instance, uint index,
                                  const matrix_t* matrix) {
	FOUNDATION_UNUSED(backend, buffer, instance, index, matrix);
}

static void
rb_dx12_buffer_set_label(render_backend_t* backend, render_buffer_t* buffer, const char* name, size_t length) {
	FOUNDATION_UNUSED(backend, buffer, name, length);
}

static render_backend_vtable_t render_backend_vtable_null = {
    .construct = rb_dx12_construct,
    .destruct = rb_dx12_destruct,
    .enumerate_adapters = rb_dx12_enumerate_adapters,
    .enumerate_modes = rb_dx12_enumerate_modes,
    .target_window_allocate = rb_dx12_target_window_allocate,
    .target_texture_allocate = rb_dx12_target_texture_allocate,
    .target_deallocate = rb_dx12_target_deallocate,
    .pipeline_allocate = rb_dx12_pipeline_allocate,
    .pipeline_deallocate = rb_dx12_pipeline_deallocate,
    .pipeline_set_color_attachment = rb_dx12_pipeline_set_color_attachment,
    .pipeline_set_depth_attachment = rb_dx12_pipeline_set_depth_attachment,
    .pipeline_set_color_clear = rb_dx12_pipeline_set_color_clear,
    .pipeline_set_depth_clear = rb_dx12_pipeline_set_depth_clear,
    .pipeline_build = rb_dx12_pipeline_build,
    .pipeline_flush = rb_dx12_pipeline_flush,
    .pipeline_use_argument_buffer = rb_dx12_pipeline_use_argument_buffer,
    .pipeline_use_render_buffer = rb_dx12_pipeline_use_render_buffer,
    .pipeline_state_allocate = rb_dx12_pipeline_state_allocate,
    .pipeline_state_deallocate = rb_dx12_pipeline_state_deallocate,
    .shader_upload = rb_dx12_shader_upload,
    .shader_finalize = rb_dx12_shader_finalize,
    .buffer_allocate = rb_dx12_buffer_allocate,
    .buffer_deallocate = rb_dx12_buffer_deallocate,
    .buffer_upload = rb_dx12_buffer_upload,
    .buffer_set_label = rb_dx12_buffer_set_label,
    .buffer_data_declare = rb_dx12_buffer_data_declare,
    .buffer_data_encode_buffer = rb_dx12_buffer_data_encode_buffer,
    .buffer_data_encode_matrix = rb_dx12_buffer_data_encode_matrix,
    .buffer_data_encode_constant = rb_dx12_buffer_data_encode_constant};

render_backend_t*
render_backend_directx12_allocate(void) {
	render_backend_t* backend =
	    memory_allocate(HASH_RENDER, sizeof(render_backend_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_DIRECTX12;
	backend->api_group = RENDERAPIGROUP_DIRECTX;
	backend->vtable = render_backend_vtable_null;
	return backend;
}

#else

render_backend_t*
render_backend_directx12_allocate(void) {
	return nullptr;
}

#endif
