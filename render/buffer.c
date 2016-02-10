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

int
render_buffer_initialize(void) {
	memory_context_push(HASH_RENDER);
	_render_map_buffer = objectmap_allocate(_render_config.buffer_max);
	memory_context_pop();
	return 0;
}

void
render_buffer_finalize(void) {
	objectmap_deallocate(_render_map_buffer);
	_render_map_buffer = 0;
}

object_t
render_buffer_ref(object_t id) {
	int32_t ref;
	render_buffer_t* buffer = objectmap_lookup(_render_map_buffer, id);
	if (buffer)
		do {
			ref = atomic_load32(&buffer->ref);
			if ((ref > 0) && atomic_cas32(&buffer->ref, ref + 1, ref))
				return id;
		}
		while (ref > 0);
	return 0;
}

void
render_buffer_destroy(object_t id) {
	int32_t ref;
	render_buffer_t* buffer = GET_BUFFER(id);
	if (buffer) {
		do {
			ref = atomic_load32(&buffer->ref);
			if ((ref > 0) && atomic_cas32(&buffer->ref, ref - 1, ref)) {
				if (ref == 1) {
					objectmap_free(_render_map_buffer, id);
					buffer->backend->vtable.deallocate_buffer(buffer->backend, buffer, true, true);
					memory_deallocate(buffer);
				}
				return;
			}
		}
		while (ref > 0);
	}
}

void
render_buffer_upload(render_buffer_t* buffer) {
	if (buffer->flags & RENDERBUFFER_DIRTY)
		buffer->backend->vtable.upload_buffer(buffer->backend, (render_buffer_t*)buffer);
}

void
render_buffer_lock(object_t id, unsigned int lock) {
	render_buffer_t* buffer = GET_BUFFER(id);
	if (render_buffer_ref(id) != id)
		return;
	if (lock & RENDERBUFFER_LOCK_WRITE) {
		atomic_incr32(&buffer->locks);
		buffer->access = buffer->store;
	}
	else if (lock & RENDERBUFFER_LOCK_READ) {
		atomic_incr32(&buffer->locks);
		buffer->access = buffer->store;
	}
	buffer->flags |= (lock & RENDERBUFFER_LOCK_BITS);
}

void
render_buffer_unlock(object_t id) {
	render_buffer_t* buffer = GET_BUFFER(id);
	if (!atomic_load32(&buffer->locks))
		return;
	if (atomic_decr32(&buffer->locks) == 0) {
		buffer->access = nullptr;
		if ((buffer->flags & RENDERBUFFER_LOCK_WRITE) && !(buffer->flags & RENDERBUFFER_LOCK_NOUPLOAD)) {
			buffer->flags |= RENDERBUFFER_DIRTY;
			if ((buffer->policy == RENDERBUFFER_UPLOAD_ONUNLOCK) ||
			        (buffer->flags & RENDERBUFFER_LOCK_FORCEUPLOAD))
				render_buffer_upload(buffer);
		}
		buffer->flags &= ~RENDERBUFFER_LOCK_BITS;
	}
	render_buffer_destroy(id);
}
