/* parameter.c  -  Render library  -  Public Domain  -  2015 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The dependent library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

render_parameter_decl_t*
render_parameter_decl_allocate(size_t num) {
	render_parameter_decl_t* decl;
	size_t size = sizeof(render_parameter_decl_t) + sizeof(render_parameter_t) * num;
	decl = memory_allocate(HASH_RENDER, size, 0, MEMORY_PERSISTENT);
	render_parameter_decl_initialize(decl, num);
	return decl;
}

void
render_parameter_decl_initialize(render_parameter_decl_t* decl, size_t num) {
	decl->num_parameters = num;
}

void
render_parameter_decl_finalize(render_parameter_decl_t* decl) {
}

void
render_parameter_decl_deallocate(render_parameter_decl_t* decl) {
	if (decl)
		render_parameter_decl_finalize(decl);
	memory_deallocate(decl);
}

object_t
render_parameterbuffer_create(render_backend_t* backend, const render_parameter_decl_t* decl,
                              const void* data) {
	return 0;
}

object_t
render_parameterbuffer_ref(object_t buffer) {
	return 0;
}

void
render_parameterbuffer_destroy(object_t buffer) {
}

void
render_parameterbuffer_link(object_t buffer, render_program_t* program) {
}

const render_parameter_decl_t*
render_parameterbuffer_decl(object_t buffer) {
	return nullptr;
}

void
render_parameterbuffer_lock(object_t buffer, unsigned int lock) {
}

void
render_parameterbuffer_unlock(object_t buffer) {
}

render_buffer_uploadpolicy_t
render_parameterbuffer_upload_policy(object_t buffer) {
	return RENDERBUFFER_UPLOAD_ONUNLOCK;
}

void
render_parameterbuffer_set_upload_policy(object_t buffer, render_buffer_uploadpolicy_t policy) {
}

void
render_parameterbuffer_upload(object_t buffer) {
}

void
render_parameterbuffer_release(object_t buffer, bool sys, bool aux) {
}

void
render_parameterbuffer_restore(object_t buffer) {
}
