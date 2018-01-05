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

static void
_render_target_deallocate(void* ptr) {
	memory_deallocate(ptr);
}

int
render_target_initialize(void) {
	memory_context_push(HASH_RENDER);
	_render_map_target = objectmap_allocate(_render_config.target_max);
	memory_context_pop();
	return 0;
}

void
render_target_finalize(void) {
	objectmap_deallocate(_render_map_target);
	_render_map_target = 0;
}

static render_target_t*
render_target_allocate(object_t* id) {
	render_target_t* target = nullptr;

	memory_context_push(HASH_RENDER);

	*id = objectmap_reserve(_render_map_target);
	if (*id) {
		target = memory_allocate(HASH_RENDER, sizeof(render_target_t), 0,
		                         MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		objectmap_set(_render_map_target, *id, target);
	}
	else {
		log_error(HASH_RENDER, ERROR_OUT_OF_MEMORY,
		          STRING_CONST("Unable to create render target, out of slots in object map"));
	}

	memory_context_pop();

	return target;
}

object_t
render_target_create(render_backend_t* backend, unsigned int width, unsigned int height,
                     pixelformat_t pixelformat, colorspace_t colorspace) {
	object_t id = 0;
	render_target_t* target = render_target_allocate(&id);

	if (target) {
		target->width = width;
		target->height = height;
		target->pixelformat = pixelformat;
		target->colorspace = colorspace;
		if (backend->vtable.allocate_target(backend, target)) {
			target->backend = backend;
		}
		else {
			render_target_release(id);
			id = 0;
		}
	}

	return id;
}

object_t
render_target_create_framebuffer(render_backend_t* backend) {
	object_t id = 0;
	render_target_t* target = render_target_allocate(&id);
	
	if (target) {
		target->backend = backend;
		target->width = 0;
		target->height = 0;
		target->pixelformat = PIXELFORMAT_R8G8B8;
		target->colorspace = COLORSPACE_LINEAR;
	}

	return id;
}

render_target_t*
render_target_acquire(object_t id) {
	return objectmap_acquire(_render_map_target, id);
}

void
render_target_release(object_t id) {
	objectmap_release(_render_map_target, id, _render_target_deallocate);
}

render_target_t*
render_target_lookup(object_t id) {
	return objectmap_lookup(_render_map_target, id);
}

unsigned int
render_target_width(object_t id) {
	render_target_t* target = objectmap_lookup(_render_map_target, id);
	if (!target)
		return 0;
	return target->width;
}

unsigned int
render_target_height(object_t id) {
	render_target_t* target = objectmap_lookup(_render_map_target, id);
	if (!target)
		return 0;
	return target->height;
}

pixelformat_t
render_target_pixelformat(object_t id) {
	render_target_t* target = objectmap_lookup(_render_map_target, id);
	if (!target)
		return PIXELFORMAT_INVALID;
	return target->pixelformat;
}

colorspace_t
render_target_colorspace(object_t id) {
	render_target_t* target = objectmap_lookup(_render_map_target, id);
	if (!target)
		return COLORSPACE_INVALID;
	return target->colorspace;
}

