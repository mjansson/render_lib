/* postprocess.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

#include <render/postprocess.h>
#include <render/hashstrings.h>

#include <foundation/memory.h>

render_postprocess_t*
render_postprocess_allocate(void) {
	render_postprocess_t* postprocess = memory_allocate(HASH_RENDER, sizeof(render_postprocess_t), 0, MEMORY_PERSISTENT);
	render_postprocess_initialize(postprocess);
	return postprocess;
}

void
render_postprocess_initialize(render_postprocess_t* postprocess) {
	memset(postprocess, 0, sizeof(render_postprocess_t));
}

void
render_postprocess_finalize(render_postprocess_t* postprocess) {
	FOUNDATION_UNUSED(postprocess);
}

void
render_postprocess_deallocate(render_postprocess_t* postprocess) {
	render_postprocess_finalize(postprocess);
	memory_deallocate(postprocess);
}

void
render_postprocess_process(render_postprocess_t* postprocess) {
	FOUNDATION_UNUSED(postprocess);
}
