/* indexbuffer.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

object_t
render_indexbuffer_create(render_backend_t* backend, render_usage_t usage, size_t indices,
                          const uint16_t* data) {
	object_t id = objectmap_reserve(_render_map_buffer);
	if (!id) {
		log_error(HASH_RENDER, ERROR_OUT_OF_MEMORY,
		          STRING_CONST("Unable to create index buffer, out of slots in object map"));
		return 0;
	}

	memory_context_push(HASH_RENDER);

	render_indexbuffer_t* buffer = memory_allocate(HASH_RENDER, sizeof(render_indexbuffer_t), 0,
	                                               MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	buffer->backend    = backend;
	buffer->usage      = usage;
	buffer->buffertype = RENDERBUFFER_INDEX;
	buffer->policy     = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->size       = 2;
	semaphore_initialize(&buffer->lock, 1);
	objectmap_set(_render_map_buffer, id, buffer);

	if (indices) {
		buffer->allocated = indices;
		buffer->used = indices;
		buffer->store = backend->vtable.allocate_buffer(backend, (render_buffer_t*)buffer);
		if (data) {
			memcpy(buffer->store, data, indices * buffer->size);
			buffer->flags |= RENDERBUFFER_DIRTY;
		}
	}

	memory_context_pop();

	return id;
}

object_t
render_indexbuffer_load(const uuid_t uuid) {
	FOUNDATION_UNUSED(uuid);
	return 0;
}

render_indexbuffer_t*
render_indexbuffer_acquire(object_t id) {
	return (render_indexbuffer_t*)render_buffer_acquire(id);
}

void
render_indexbuffer_release(object_t id) {
	render_buffer_release(id);
}

render_usage_t
render_indexbuffer_usage(object_t id) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? (render_usage_t)buffer->usage : RENDERUSAGE_INVALID;
}

size_t
render_indexbuffer_num_allocated(object_t id) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->allocated : 0;
}

size_t
render_indexbuffer_num_elements(object_t id) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->used : 0;
}

void
render_indexbuffer_set_num_elements(object_t id, size_t num) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer) {
		buffer->used = (buffer->allocated < num) ? buffer->allocated : num;
		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}

void
render_indexbuffer_lock(object_t id, unsigned int lock) {
	render_buffer_lock(id, lock);
}

void
render_indexbuffer_unlock(object_t id) {
	render_buffer_unlock(id);
}

render_buffer_uploadpolicy_t
render_indexbuffer_upload_policy(object_t id) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? (render_buffer_uploadpolicy_t)buffer->policy : RENDERBUFFER_UPLOAD_ONDISPATCH;
}

void
render_indexbuffer_set_upload_policy(object_t id, render_buffer_uploadpolicy_t policy) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->policy = policy;
}

void
render_indexbuffer_upload(object_t id) {
	render_buffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		render_buffer_upload(buffer);
}

uint16_t*
render_indexbuffer_element(object_t id, size_t element) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	return pointer_offset(buffer->access, buffer->size * element);
}

size_t
render_indexbuffer_element_size(object_t id) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->size : 0;
}

void
render_indexbuffer_free(object_t id, bool sys, bool aux) {
	render_indexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_indexbuffer_restore(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer) {
		buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

		//...
		//All loadable resources should have a stream identifier, an offset and a size
		//to be able to repoen the stream and read the raw buffer back
		//...

		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}
