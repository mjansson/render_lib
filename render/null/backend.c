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

#include <render/null/backend.h>

static bool
rb_null_construct(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
	log_debug(HASH_RENDER, STRING_CONST("Constructed NULL render backend"));
	return true;
}

static void
rb_null_destruct(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
	log_debug(HASH_RENDER, STRING_CONST("Destructed NULL render backend"));
}

static size_t
rb_null_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	if (capacity)
		store[0] = (unsigned int)WINDOW_ADAPTER_DEFAULT;
	return 1;
}

static size_t
rb_null_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(adapter);
	if (capacity) {
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, COLORSPACE_LINEAR, 60};
		store[0] = mode;
	}
	return 1;
}

static bool
rb_null_set_drawable(render_backend_t* backend, const render_drawable_t* drawable) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(drawable);
	return true;
}

static void
rb_null_dispatch(render_backend_t* backend, render_target_t* target, render_context_t** contexts,
                 size_t contexts_count) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
	FOUNDATION_UNUSED(contexts);
	FOUNDATION_UNUSED(contexts_count);
}

static void
rb_null_flip(render_backend_t* backend) {
	++backend->framecount;
}

static void*
rb_null_allocate_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	return memory_allocate(HASH_RENDER, buffer->buffersize, 16, MEMORY_PERSISTENT);
}

static void
rb_null_deallocate_buffer(render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(aux);
	if (sys)
		memory_deallocate(buffer->store);
}

static bool
rb_null_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	return true;
}

static bool
rb_null_upload_shader(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(shader);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	return true;
}

static bool
rb_null_upload_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(program);
	return true;
}

static bool
rb_null_upload_texture(render_backend_t* backend, render_texture_t* texture, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(texture);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	return true;
}

static void
rb_null_deallocate_texture(render_backend_t* backend, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(texture);
}

static void
rb_null_parameter_bind_texture(render_backend_t* backend, void* buffer, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(texture);
}

static void
rb_null_parameter_bind_target(render_backend_t* backend, void* buffer, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(target);
}

static void
rb_null_link_buffer(render_backend_t* backend, render_buffer_t* buffer, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(program);
}

static void
rb_null_deallocate_shader(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(shader);
}

static void
rb_null_deallocate_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(program);
}

static bool
rb_null_allocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
	return true;
}

static bool
rb_null_resize_target(render_backend_t* backend, render_target_t* target, unsigned int width, unsigned int height) {
	FOUNDATION_UNUSED(backend);
	if (target) {
		target->width = width;
		target->height = height;
	}
	return true;
}

static void
rb_null_deallocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(target);
}

static void
rb_null_enable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static void
rb_null_disable_thread(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);
}

static render_backend_vtable_t render_backend_vtable_null = {.construct = rb_null_construct,
                                                             .destruct = rb_null_destruct,
                                                             .enumerate_adapters = rb_null_enumerate_adapters,
                                                             .enumerate_modes = rb_null_enumerate_modes,
                                                             .set_drawable = rb_null_set_drawable,
                                                             .enable_thread = rb_null_enable_thread,
                                                             .disable_thread = rb_null_disable_thread,
                                                             .dispatch = rb_null_dispatch,
                                                             .flip = rb_null_flip,
                                                             .allocate_buffer = rb_null_allocate_buffer,
                                                             .upload_buffer = rb_null_upload_buffer,
                                                             .upload_shader = rb_null_upload_shader,
                                                             .upload_program = rb_null_upload_program,
                                                             .upload_texture = rb_null_upload_texture,
                                                             .parameter_bind_texture = rb_null_parameter_bind_texture,
                                                             .parameter_bind_target = rb_null_parameter_bind_target,
                                                             .link_buffer = rb_null_link_buffer,
                                                             .deallocate_buffer = rb_null_deallocate_buffer,
                                                             .deallocate_shader = rb_null_deallocate_shader,
                                                             .deallocate_program = rb_null_deallocate_program,
                                                             .deallocate_texture = rb_null_deallocate_texture,
                                                             .allocate_target = rb_null_allocate_target,
                                                             .resize_target = rb_null_resize_target,
                                                             .deallocate_target = rb_null_deallocate_target};

render_backend_t*
render_backend_null_allocate(void) {
	render_backend_t* backend =
	    memory_allocate(HASH_RENDER, sizeof(render_backend_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_NULL;
	backend->api_group = RENDERAPIGROUP_NONE;
	backend->vtable = render_backend_vtable_null;
	return backend;
}
