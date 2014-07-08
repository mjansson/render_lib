/* sort.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */


#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>


void render_sort_merge( render_context_t** contexts, unsigned int num_contexts )
{
	for( unsigned int i = 0, size = num_contexts; i < size; ++i )
		contexts[i]->order = radixsort( contexts[i]->sort, contexts[i]->keys, (radixsort_index_t)atomic_load32( &contexts[i]->reserved ) );
}


void render_sort_reset( render_context_t* context )
{
	atomic_store64( &context->key, 0 );
}


uint64_t render_sort_sequential_key( render_context_t* context )
{
	return (uint64_t)atomic_incr64( &context->key );
}


uint64_t render_sort_render_key( render_context_t* context, uint64_t vertexbuffer, uint64_t indexbuffer, uint64_t blend_state )
{
	return render_sort_sequential_key( context );
}
