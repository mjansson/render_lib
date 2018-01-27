/* target.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

render_target_t*
render_target_allocate(render_backend_t* backend, unsigned int width, unsigned int height,
                       pixelformat_t pixelformat, colorspace_t colorspace) {
	render_target_t* target = memory_allocate(HASH_RENDER, sizeof(render_target_t), 0, MEMORY_PERSISTENT);
	render_target_initialize(target, backend, width, height, pixelformat, colorspace);
	return target;
}

void
render_target_initialize(render_target_t* target, render_backend_t* backend,
                         unsigned int width, unsigned int height,
                         pixelformat_t pixelformat, colorspace_t colorspace) {
	target->width = width;
	target->height = height;
	target->pixelformat = pixelformat;
	target->colorspace = colorspace;
	if (backend->vtable.allocate_target(backend, target))
		target->backend = backend;
	else
		render_target_finalize(target);
}

void
render_target_finalize(render_target_t* target) {
	if (target->backend)
		target->backend->vtable.deallocate_target(target->backend, target);
	target->backend = 0;
}

void
render_target_deallocate(render_target_t* target) {
	render_target_finalize(target);
	memory_deallocate(target);
}

void
render_target_initialize_framebuffer(render_target_t* target, render_backend_t* backend) {
	target->backend = backend;
	target->width = 0;
	target->height = 0;
	target->pixelformat = PIXELFORMAT_R8G8B8;
	target->colorspace = COLORSPACE_LINEAR;
}
