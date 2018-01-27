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

void
render_buffer_deallocate(render_buffer_t* buffer) {
	buffer->backend->vtable.deallocate_buffer(buffer->backend, buffer, true, true);
	semaphore_finalize(&buffer->lock);
	memory_deallocate(buffer);
}

void
render_buffer_upload(render_buffer_t* buffer) {
	if (buffer->flags & RENDERBUFFER_DIRTY)
		buffer->backend->vtable.upload_buffer(buffer->backend, (render_buffer_t*)buffer);
}

void
render_buffer_lock(render_buffer_t* buffer, unsigned int lock) {
	semaphore_wait(&buffer->lock);
	{
		buffer->locks++;
		buffer->access = buffer->store;
		buffer->flags |= (lock & RENDERBUFFER_LOCK_BITS);
	}
	semaphore_post(&buffer->lock);
}

void
render_buffer_unlock(render_buffer_t* buffer) {
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
}
