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
_render_target_deallocate(object_t id, void* ptr) {
	memory_deallocate(ptr);
	objectmap_free(_render_map_target, id);
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

object_t
render_target_create(render_backend_t* backend) {
	render_target_t* target;
	object_t id;

	memory_context_push(HASH_RENDER);

	id = objectmap_reserve(_render_map_target);
	if (!id) {
		log_error(HASH_RENDER, ERROR_OUT_OF_MEMORY,
		          STRING_CONST("Unable to create render target, out of slots in object map"));
		return 0;
	}

	target = memory_allocate(HASH_RENDER, sizeof(render_target_t), 0,
	                         MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	target->id = id;
	target->backend = backend;
	atomic_store32(&target->ref, 1, memory_order_release);

	objectmap_set(_render_map_target, id, target);

	return id;
}

object_t
render_target_create_framebuffer(render_backend_t* backend) {
	return render_target_create(backend);
}

object_t
render_target_ref(object_t id) {
	return objectmap_lookup_ref(_render_map_target, id) ? id : 0;
}

void
render_target_unref(object_t id) {
	objectmap_lookup_unref(_render_map_target, id, _render_target_deallocate);
}

render_target_t*
render_target_lookup(object_t id) {
	return objectmap_lookup(_render_map_target, id);
}

int
render_target_width(object_t id) {
	render_target_t* target = objectmap_lookup(_render_map_target, id);
	if (!target)
		return 0;
	return target->width;
}

int
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

