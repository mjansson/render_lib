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

render_indexbuffer_t*
render_indexbuffer_allocate(render_backend_t* backend, render_usage_t usage, size_t indices,
                            const uint16_t* data) {
	render_indexbuffer_t* buffer = memory_allocate(HASH_RENDER, sizeof(render_indexbuffer_t), 0,
	                                               MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	buffer->backend    = backend;
	buffer->usage      = (uint8_t)usage;
	buffer->buffertype = RENDERBUFFER_INDEX;
	buffer->policy     = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->size       = 2;
	semaphore_initialize(&buffer->lock, 1);

	if (indices) {
		buffer->allocated = indices;
		buffer->used = indices;
		buffer->store = backend->vtable.allocate_buffer(backend, (render_buffer_t*)buffer);
		if (data) {
			memcpy(buffer->store, data, indices * buffer->size);
			buffer->flags |= RENDERBUFFER_DIRTY;
		}
	}

	return buffer;
}

void
render_indexbuffer_deallocate(render_indexbuffer_t* buffer) {
	render_buffer_deallocate((render_buffer_t*)buffer);
}

render_indexbuffer_t*
render_indexbuffer_load(const uuid_t uuid) {
	FOUNDATION_UNUSED(uuid);
	return 0;
}

void
render_indexbuffer_lock(render_indexbuffer_t* buffer, unsigned int lock) {
	render_buffer_lock((render_buffer_t*)buffer, lock);
}

void
render_indexbuffer_unlock(render_indexbuffer_t* buffer) {
	render_buffer_unlock((render_buffer_t*)buffer);
}

void
render_indexbuffer_upload(render_indexbuffer_t* buffer) {
	render_buffer_upload((render_buffer_t*)buffer);
}

void
render_indexbuffer_free(render_indexbuffer_t* buffer, bool sys, bool aux) {
	buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_indexbuffer_restore(render_indexbuffer_t* buffer) {
	buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

	//...
	//All loadable resources should have a stream identifier, an offset and a size
	//to be able to repoen the stream and read the raw buffer back
	//...

	buffer->flags |= RENDERBUFFER_DIRTY;
}
