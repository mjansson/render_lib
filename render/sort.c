/* sort.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

void
render_sort_merge(render_context_t** contexts, size_t num_contexts) {
	for (size_t i = 0, size = num_contexts; i < size; ++i)
		contexts[i]->order =
		    radixsort_sort(contexts[i]->sort, contexts[i]->keys,
		                   atomic_load32(&contexts[i]->reserved, memory_order_acquire));
}

void
render_sort_reset(render_context_t* context) {
	atomic_store64(&context->key, 0, memory_order_release);
}

uint64_t
render_sort_sequential_key(render_context_t* context) {
	return (uint64_t)atomic_incr64(&context->key, memory_order_release);
}

uint64_t
render_sort_render_key(render_context_t* context, render_buffer_t* vertexbuffer,
                       render_buffer_t* indexbuffer, render_state_t* state) {
	FOUNDATION_UNUSED(vertexbuffer);
	FOUNDATION_UNUSED(indexbuffer);
	FOUNDATION_UNUSED(state);
	return render_sort_sequential_key(context);
}
