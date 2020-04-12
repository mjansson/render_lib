/* parameter.c  -  Render library  -  Public Domain  -  2015 Mattias Jansson
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

#include <render/render.h>
#include <render/internal.h>

void
render_parameterbuffer_initialize(render_parameterbuffer_t* parameterbuffer, render_backend_t* backend,
                                  render_usage_t usage, const render_parameter_t* parameters, size_t parameters_count,
                                  const void* data, size_t data_size) {
	parameterbuffer->backend = backend;
	parameterbuffer->usage = (uint8_t)usage;
	parameterbuffer->buffertype = RENDERBUFFER_PARAMETER;
	parameterbuffer->policy = RENDERBUFFER_UPLOAD_ONDISPATCH;
	parameterbuffer->buffersize = data_size;
	parameterbuffer->parameter_count = (unsigned int)parameters_count;
	semaphore_initialize(&parameterbuffer->lock, 1);
	if (parameters) {
		memcpy(&parameterbuffer->parameters, parameters, sizeof(render_parameter_t) * parameters_count);
	}
	memset(parameterbuffer->backend_data, 0, sizeof(parameterbuffer->backend_data));

	parameterbuffer->allocated = 1;
	parameterbuffer->used = 1;
	parameterbuffer->store = backend->vtable.allocate_buffer(backend, (render_buffer_t*)parameterbuffer);
	if (data) {
		memcpy(parameterbuffer->store, data, data_size);
		parameterbuffer->flags |= RENDERBUFFER_DIRTY;
	}
}

render_parameterbuffer_t*
render_parameterbuffer_allocate(render_backend_t* backend, render_usage_t usage, const render_parameter_t* parameters,
                                size_t parameters_count, const void* data, size_t data_size) {
	render_parameterbuffer_t* parameterbuffer =
	    memory_allocate(HASH_RENDER, sizeof(render_parameterbuffer_t) + (sizeof(render_parameter_t) * parameters_count),
	                    0, MEMORY_PERSISTENT);
	render_parameterbuffer_initialize(parameterbuffer, backend, usage, parameters, parameters_count, data, data_size);
	return parameterbuffer;
}

void
render_parameterbuffer_deallocate(render_parameterbuffer_t* buffer) {
	render_buffer_deallocate((render_buffer_t*)buffer);
}

void
render_parameterbuffer_link(render_parameterbuffer_t* buffer, render_program_t* program) {
	buffer->backend->vtable.link_buffer(buffer->backend, (render_buffer_t*)buffer, program);
}

void
render_parameterbuffer_lock(render_parameterbuffer_t* buffer, unsigned int lock) {
	render_buffer_lock((render_buffer_t*)buffer, lock);
}

void
render_parameterbuffer_unlock(render_parameterbuffer_t* buffer) {
	render_buffer_unlock((render_buffer_t*)buffer);
}

void
render_parameterbuffer_upload(render_parameterbuffer_t* buffer) {
	render_buffer_upload((render_buffer_t*)buffer);
}

void
render_parameterbuffer_free(render_parameterbuffer_t* buffer, bool sys, bool aux) {
	buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_parameterbuffer_restore(render_parameterbuffer_t* buffer) {
	buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

	//...
	// All loadable resources should have a stream identifier, an offset and a size
	// to be able to repoen the stream and read the raw buffer back
	//...

	buffer->flags |= RENDERBUFFER_DIRTY;
}
