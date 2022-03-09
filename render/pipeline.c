/* pipeline.c  -  Render library  -  Public Domain  -  2017 Mattias Jansson
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

#include <render/pipeline.h>
#include <render/backend.h>
#include <render/hashstrings.h>

render_pipeline_t*
render_pipeline_allocate(render_backend_t* backend) {
	return backend->vtable.pipeline_allocate(backend);
}

void
render_pipeline_deallocate(render_pipeline_t* pipeline) {
	if (pipeline && pipeline->backend)
		pipeline->backend->vtable.pipeline_deallocate(pipeline->backend, pipeline);
}

void
render_pipeline_set_color_attachment(render_pipeline_t* pipeline, uint slot, render_target_t* target) {
	if (pipeline && pipeline->backend)
		pipeline->backend->vtable.pipeline_set_color_attachment(pipeline->backend, pipeline, slot, target);
}

void
render_pipeline_set_depth_attachment(render_pipeline_t* pipeline, render_target_t* target) {
	if (pipeline && pipeline->backend)
		pipeline->backend->vtable.pipeline_set_depth_attachment(pipeline->backend, pipeline, target);
}

void
render_pipeline_set_color_clear(render_pipeline_t* pipeline, uint slot, render_clear_action_t action, vector_t color) {
	if (pipeline && pipeline->backend)
		pipeline->backend->vtable.pipeline_set_color_clear(pipeline->backend, pipeline, slot, action, color);
}

void
render_pipeline_set_depth_clear(render_pipeline_t* pipeline, render_clear_action_t action, vector_t color) {
	if (pipeline && pipeline->backend)
		pipeline->backend->vtable.pipeline_set_depth_clear(pipeline->backend, pipeline, action, color);
}

void
render_pipeline_flush(render_pipeline_t* pipeline) {
	if (pipeline && pipeline->backend)
		pipeline->backend->vtable.pipeline_flush(pipeline->backend, pipeline);
}
