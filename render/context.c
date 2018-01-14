/* context.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

render_context_t*
render_context_allocate(size_t commands) {
	render_context_t* context;

	FOUNDATION_ASSERT(commands < (radixsort_index_t) - 1);

	memory_context_push(HASH_RENDER);

	context = memory_allocate(HASH_RENDER, sizeof(render_context_t), 0,
	                          MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	context->allocated = (int32_t)commands;
	context->commands  = memory_allocate(HASH_RENDER, sizeof(render_command_t) * commands, 0,
	                                     MEMORY_PERSISTENT);
	context->keys      = memory_allocate(HASH_RENDER, sizeof(uint64_t) * commands, 0,
	                                     MEMORY_PERSISTENT);
	context->sort      = radixsort_allocate(RADIXSORT_UINT64, (radixsort_index_t)commands);

	memory_context_pop();

	return context;
}

void
render_context_deallocate(render_context_t* context) {
	if (context) {
		radixsort_deallocate(context->sort);
		memory_deallocate(context->commands);
		memory_deallocate(context->keys);
		memory_deallocate(context);
	}
}

render_command_t*
render_context_reserve(render_context_t* context, uint64_t sort) {
	int32_t idx = atomic_exchange_and_add32(&context->reserved, 1, memory_order_relaxed);
	FOUNDATION_ASSERT_MSG(idx < context->allocated, "Render command overallocation");
	context->keys[ idx ] = sort;
	return context->commands + idx;
}

void
render_context_queue(render_context_t* context, render_command_t* command, uint64_t sort) {
	int32_t idx = atomic_exchange_and_add32(&context->reserved, 1, memory_order_relaxed);
	FOUNDATION_ASSERT_MSG(idx < context->allocated, "Render command overallocation");
	context->keys[ idx ] = sort;
	memcpy(context->commands + idx, command, sizeof(render_command_t));
}

size_t
render_context_reserved(render_context_t* context) {
	return (size_t)atomic_load32(&context->reserved, memory_order_acquire);
}

uint8_t
render_context_group(render_context_t* context) {
	return context->group;
}

void
render_context_set_group(render_context_t* context, uint8_t group) {
	context->group = group;
}
