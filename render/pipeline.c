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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <render/pipeline.h>
#include <render/context.h>
#include <render/sort.h>
#include <render/backend.h>
#include <render/hashstrings.h>

#include <foundation/memory.h>
#include <foundation/array.h>

#include <task/scheduler.h>

render_pipeline_t*
render_pipeline_allocate(render_backend_t* backend) {
	render_pipeline_t* pipeline =
	    memory_allocate(HASH_RENDER, sizeof(render_pipeline_t), 0, MEMORY_PERSISTENT);
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
	if (pipeline) {
		for (size_t istep = 0, ssize = array_size(pipeline->steps); istep < ssize; ++istep)
			render_pipeline_step_finalize(pipeline->steps + istep);
		array_deallocate(pipeline->steps);
	}
}

void
render_pipeline_deallocate(render_pipeline_t* pipeline) {
	render_pipeline_finalize(pipeline);
	memory_deallocate(pipeline);
}

static task_return_t
render_pipeline_execute_step(task_arg_t arg) {
	render_pipeline_step_t* step = arg;
	size_t context_count =

	    step->executor(step->backend, step->target, step->contexts, array_size(step->contexts));

	render_sort_merge(step->contexts, num_contexts);
}

void
render_pipeline_execute(render_pipeline_t* pipeline) {
	if (pipeline->scheduler) {
		size_t step_count = array_size(pipeline->steps);
		array_resize(pipeline->step_task, step_count);
		array_resize(pipeline->step_arg, step_count);
		for (size_t istep = 0; istep < step_count; ++istep) {
			pipeline->step_task[istep].function = render_pipeline_execute_step;
			pipeline->step_task[istep].name =
			    string_const(STRING_CONST("render_pipeline_execute_step"));
			pipeline->step_arg[istep] = pipeline->steps + istep;
		}
		task_scheduler_multiqueue(pipeline->scheduler, step_count, pipeline->step_task,
		                          pipeline->step_arg, 0);
	} else {
		for (size_t istep = 0, ssize = array_size(pipeline->steps); istep < ssize; ++istep) {
			render_pipeline_step_t* step = pipeline->steps + istep;
			step->executor(pipeline->backend, step->target, step->contexts,
			               array_size(step->contexts));

			render_sort_merge(step->contexts, num_contexts);
			render_backend_dispatch(pipeline->backend, step->target, step->contexts, num_contexts);
		}
	}
}

void
render_pipeline_step_initialize(render_pipeline_step_t* step, render_target_t* target,
                                render_pipeline_execute_fn executor) {
	memset(step, 0, sizeof(render_pipeline_step_t));
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

void
render_pipeline_step_blit_initialize(render_pipeline_step_t* step, render_target_t* target_source,
                                     render_target_t* target_destination) {
	memset(step, 0, sizeof(render_pipeline_step_t));
	step->target = target_destination;
	// step->contexts = nullptr;
	// step->executor = executor;

	FOUNDATION_UNUSED(target_source);
}
