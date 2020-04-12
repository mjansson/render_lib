/* state.c  -  Render library  -  Public Domain  -  2015 Mattias Jansson
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

render_state_t
render_state_default(void) {
	render_state_t state = {0};
	state.blend_source_color = BLEND_ONE;
	state.blend_source_alpha = BLEND_ONE;
	state.depth_func = RENDER_CMP_LESSEQUAL;
	state.target_write[0] = true;
	state.depth_write = true;
	return state;
}

render_statebuffer_t*
render_statebuffer_allocate(render_backend_t* backend, render_usage_t usage, const render_state_t state) {
	render_statebuffer_t* statebuffer =
	    memory_allocate(HASH_RENDER, sizeof(render_statebuffer_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	render_statebuffer_initialize(statebuffer, backend, usage, state);
	return statebuffer;
}

void
render_statebuffer_initialize(render_statebuffer_t* buffer, render_backend_t* backend, render_usage_t usage,
                              const render_state_t state) {
	buffer->backend = backend;
	buffer->usage = (uint8_t)usage;
	buffer->buffertype = RENDERBUFFER_STATE;
	buffer->policy = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->buffersize = sizeof(render_state_t);

	buffer->allocated = 1;
	buffer->used = 1;
	buffer->state = state;
	buffer->store = &buffer->state;
}

void
render_statebuffer_deallocate(render_statebuffer_t* buffer) {
	render_buffer_deallocate((render_buffer_t*)buffer);
}

void
render_statebuffer_lock(render_statebuffer_t* buffer, unsigned int lock) {
	render_buffer_lock((render_buffer_t*)buffer, lock);
}

void
render_statebuffer_unlock(render_statebuffer_t* buffer) {
	render_buffer_unlock((render_buffer_t*)buffer);
}

render_state_t*
render_statebuffer_data(render_statebuffer_t* buffer) {
	return buffer ? buffer->access : nullptr;
}

void
render_statebuffer_upload(render_statebuffer_t* buffer) {
	render_buffer_upload((render_buffer_t*)buffer);
}

void
render_statebuffer_free(render_statebuffer_t* buffer, bool sys, bool aux) {
	buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_statebuffer_restore(render_statebuffer_t* buffer) {
	buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

	//...
	// All loadable resources should have a stream identifier, an offset and a size
	// to be able to repoen the stream and read the raw buffer back
	//...

	buffer->flags |= RENDERBUFFER_DIRTY;
}
