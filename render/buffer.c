/* buffer.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

render_buffer_t*
render_buffer_allocate(render_backend_t* backend, render_usage_t usage, size_t buffer_size, const void* data,
                       size_t data_size) {
	render_buffer_t* buffer =
	    memory_allocate(HASH_RENDER, sizeof(render_buffer_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	buffer->backend = backend;
	buffer->usage = (uint8_t)usage;
	semaphore_initialize(&buffer->lock, 1);
	memset(buffer->backend_data, 0, sizeof(buffer->backend_data));
	if (buffer_size)
		backend->vtable.buffer_allocate(backend, buffer, buffer_size, data, data_size);
	return buffer;
}

void
render_buffer_deallocate(render_buffer_t* buffer) {
	if (buffer) {
		buffer->backend->vtable.buffer_deallocate(buffer->backend, buffer, true, true);
		semaphore_finalize(&buffer->lock);
		memory_deallocate(buffer);
	}
}

void
render_buffer_upload(render_buffer_t* buffer) {
	if (buffer->flags & RENDERBUFFER_DIRTY)
		buffer->backend->vtable.buffer_upload(buffer->backend, buffer);
}

void
render_buffer_lock(render_buffer_t* buffer, unsigned int lock) {
	if (buffer->usage == RENDERUSAGE_GPUONLY)
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
render_buffer_unlock(render_buffer_t* buffer) {
	semaphore_wait(&buffer->lock);
	if (buffer->locks) {
		--buffer->locks;
		if (!buffer->locks) {
			buffer->access = nullptr;
			if (buffer->flags & RENDERBUFFER_LOCK_WRITE) {
				buffer->flags |= RENDERBUFFER_DIRTY;
				if (!(buffer->flags & RENDERBUFFER_LOCK_NOUPLOAD))
					render_buffer_upload(buffer);
			}
			buffer->flags &= ~(uint32_t)RENDERBUFFER_LOCK_BITS;
		}
	}
	semaphore_post(&buffer->lock);
}

void
render_buffer_data_declare(render_buffer_t* buffer, size_t instance_count, const render_buffer_data_t* data,
                           size_t data_count) {
	buffer->backend->vtable.buffer_data_declare(buffer->backend, buffer, instance_count, data, data_count);
}

void
render_buffer_data_encode_buffer(render_buffer_t* buffer, uint instance, uint index, render_buffer_t* source,
                                 uint offset) {
	buffer->backend->vtable.buffer_data_encode_buffer(buffer->backend, buffer, instance, index, source, offset);
}

void
render_buffer_data_encode_matrix(render_buffer_t* buffer, uint instance, uint index, const matrix_t* matrix) {
	buffer->backend->vtable.buffer_data_encode_matrix(buffer->backend, buffer, instance, index, matrix);
}

void
render_buffer_data_encode_constant(render_buffer_t* buffer, uint instance, uint index, const void* data, uint size) {
	buffer->backend->vtable.buffer_data_encode_constant(buffer->backend, buffer, instance, index, data, size);
}

void
render_buffer_set_label(render_buffer_t* buffer, const char* name, size_t length) {
#if BUILD_DEBUG || BUILD_RELEASE
	buffer->backend->vtable.buffer_set_label(buffer->backend, buffer, name, length);
#endif
	FOUNDATION_UNUSED(buffer, name, length);
}
