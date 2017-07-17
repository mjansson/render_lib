/* buffer.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

static void
_render_buffer_deallocate(void* ptr) {
	render_buffer_t* buffer = ptr;
	buffer->backend->vtable.deallocate_buffer(buffer->backend, buffer, true, true);
	semaphore_finalize(&buffer->lock);
	memory_deallocate(buffer);
}

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

render_buffer_t*
render_buffer_acquire(object_t id) {
	return objectmap_acquire(_render_map_buffer, id);
}

void
render_buffer_release(object_t id) {
	objectmap_release(_render_map_buffer, id, _render_buffer_deallocate);
}

void
render_buffer_upload(render_buffer_t* buffer) {
	if (buffer->flags & RENDERBUFFER_DIRTY)
		buffer->backend->vtable.upload_buffer(buffer->backend, (render_buffer_t*)buffer);
}

void
render_buffer_lock(object_t id, unsigned int lock) {
	render_buffer_t* buffer = render_buffer_acquire(id);
	if (!buffer)
		return;
	semaphore_wait(&buffer->lock);
	{
		buffer->locks++;
		buffer->access = buffer->store;
		buffer->flags |= (lock & RENDERBUFFER_LOCK_BITS);
	}
	semaphore_post(&buffer->lock);
}

void
render_buffer_unlock(object_t id) {
	render_buffer_t* buffer = GET_BUFFER(id);
	semaphore_wait(&buffer->lock);
	{
		if (buffer->locks) {
			--buffer->locks;
			if (!buffer->locks) {
				buffer->access = nullptr;
				if ((buffer->flags & RENDERBUFFER_LOCK_WRITE) && !(buffer->flags & RENDERBUFFER_LOCK_NOUPLOAD)) {
					buffer->flags |= RENDERBUFFER_DIRTY;
					if ((buffer->policy == RENDERBUFFER_UPLOAD_ONUNLOCK) ||
					        (buffer->flags & RENDERBUFFER_LOCK_FORCEUPLOAD))
						render_buffer_upload(buffer);
				}
				buffer->flags &= ~(uint32_t)RENDERBUFFER_LOCK_BITS;
			}
		}
	}
	semaphore_post(&buffer->lock);
	render_buffer_release(id);
}
