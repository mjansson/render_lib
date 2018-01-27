/* pipeline.c  -  Render library  -  Public Domain  -  2017 Mattias Jansson / Rampant Pixels
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

#include <render/pipeline.h>
#include <render/context.h>
#include <render/sort.h>
#include <render/backend.h>
#include <render/hashstrings.h>

#include <foundation/memory.h>
#include <foundation/array.h>

render_pipeline_t*
render_pipeline_allocate(render_backend_t* backend) {
	render_pipeline_t* pipeline = memory_allocate(HASH_RENDER, sizeof(render_pipeline_t), 0, MEMORY_PERSISTENT);
	render_pipeline_initialize(pipeline, backend);
	return pipeline;
}

void
render_pipeline_initialize(render_pipeline_t* pipeline, render_backend_t* backend) {
	memset(pipeline, 0, sizeof(render_pipeline_t));
	pipeline->backend = backend;
}

void
render_pipeline_finalize(render_pipeline_t* pipeline) {
	for (size_t istep = 0, ssize = array_size(pipeline->steps); istep < ssize; ++istep)
		render_pipeline_step_finalize(pipeline->steps + istep);
	array_deallocate(pipeline->steps);
}

void
render_pipeline_deallocate(render_pipeline_t* pipeline) {
	render_pipeline_finalize(pipeline);
	memory_deallocate(pipeline);
}

void
render_pipeline_execute(render_pipeline_t* pipeline) {
	for (size_t istep = 0, ssize = array_size(pipeline->steps); istep < ssize; ++istep) {
		render_pipeline_step_t* step = pipeline->steps + istep;
		size_t num_contexts = array_size(step->contexts);
		
		step->executor(pipeline->backend, step->target, step->contexts, num_contexts);

		render_sort_merge(step->contexts, num_contexts);
		render_backend_dispatch(pipeline->backend, step->target, step->contexts, num_contexts);
	}
}

void
render_pipeline_step_initialize(render_pipeline_step_t* step, render_target_t* target, render_pipeline_execute_fn executor) {
	step->target = target;
	step->contexts = nullptr;
	step->executor = executor;
}

void
render_pipeline_step_finalize(render_pipeline_step_t* step) {
	for (size_t icontext = 0, csize = array_size(step->contexts); icontext < csize; ++icontext)
		render_context_deallocate(step->contexts[icontext]);
	array_deallocate(step->contexts);
}
