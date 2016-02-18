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

#define GET_BUFFER(id) objectmap_lookup(_render_map_buffer, (id))

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
	decl->num_parameters = (unsigned int)num;
}

void
render_parameter_decl_finalize(render_parameter_decl_t* decl) {
	FOUNDATION_UNUSED(decl);
}

void
render_parameter_decl_deallocate(render_parameter_decl_t* decl) {
	if (decl)
		render_parameter_decl_finalize(decl);
	memory_deallocate(decl);
}

object_t
render_parameterbuffer_create(render_backend_t* backend, render_usage_t usage,
                              const render_parameter_decl_t* decl, const void* data) {
	object_t id = objectmap_reserve(_render_map_buffer);
	if (!id) {
		log_error(HASH_RENDER, ERROR_OUT_OF_MEMORY,
		          STRING_CONST("Unable to allocate parameter buffer, out of slots in object map"));
		return 0;
	}

	memory_context_push(HASH_RENDER);

	size_t paramsize = sizeof(render_parameter_t) * decl->num_parameters;
	render_parameterbuffer_t* buffer = memory_allocate(HASH_RENDER,
	                                                   sizeof(render_parameterbuffer_t) + paramsize, 0,
	                                                   MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	buffer->id         = id;
	buffer->backend    = backend;
	buffer->usage      = usage;
	buffer->buffertype = RENDERBUFFER_PARAMETER;
	buffer->policy     = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->size       = decl->size;
	memcpy(&buffer->decl, decl, sizeof(render_parameter_decl_t) + paramsize);
	atomic_store32(&buffer->ref, 1);
	objectmap_set(_render_map_buffer, id, buffer);

	buffer->allocated = 1;
	buffer->used = 1;
	buffer->store = backend->vtable.allocate_buffer(backend, (render_buffer_t*)buffer);
	if (data) {
		memcpy(buffer->store, data, decl->size);
		buffer->flags |= RENDERBUFFER_DIRTY;
	}

	memory_context_pop();

	return id;
}

object_t
render_parameterbuffer_ref(object_t id) {
	return render_buffer_ref(id);
}

void
render_parameterbuffer_destroy(object_t id) {
	render_buffer_destroy(id);
}

void
render_parameterbuffer_link(object_t id, render_program_t* program) {
	render_buffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->backend->vtable.link_buffer(buffer->backend, buffer, program);
}

const render_parameter_decl_t*
render_parameterbuffer_decl(object_t id) {
	render_parameterbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? &buffer->decl : nullptr;
}

void
render_parameterbuffer_lock(object_t id, unsigned int lock) {
	render_buffer_lock(id, lock);
}

void
render_parameterbuffer_unlock(object_t id) {
	render_buffer_unlock(id);
}

void*
render_parameterbuffer_data(object_t id) {
	render_buffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->access : nullptr;
}

render_buffer_uploadpolicy_t
render_parameterbuffer_upload_policy(object_t id) {
	render_parameterbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->policy : RENDERBUFFER_UPLOAD_ONUNLOCK;
}

void
render_parameterbuffer_set_upload_policy(object_t id, render_buffer_uploadpolicy_t policy) {
	render_parameterbuffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->policy = policy;
}

void
render_parameterbuffer_upload(object_t id) {
	render_buffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		render_buffer_upload(buffer);
}

void
render_parameterbuffer_release(object_t id, bool sys, bool aux) {
	render_parameterbuffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_parameterbuffer_restore(object_t id) {
	render_parameterbuffer_t* buffer = GET_BUFFER(id);
	if (buffer) {
		buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

		//...
		//All loadable resources should have a stream identifier, an offset and a size
		//to be able to repoen the stream and read the raw buffer back
		//...

		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}