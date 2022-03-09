/* pipeline.h  -  Render library  -  Public Domain  -  2017 Mattias Jansson
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

#pragma once

/*! \file pipeline.h
    Render pipeline */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_pipeline_t*
render_pipeline_allocate(render_backend_t* backend);

RENDER_API void
render_pipeline_deallocate(render_pipeline_t* pipeline);

RENDER_API void
render_pipeline_set_color_attachment(render_pipeline_t* pipeline, uint slot, render_target_t* target);

RENDER_API void
render_pipeline_set_depth_attachment(render_pipeline_t* pipeline, render_target_t* target);

RENDER_API void
render_pipeline_set_color_clear(render_pipeline_t* pipeline, uint slot, render_clear_action_t action, vector_t color);

RENDER_API void
render_pipeline_set_depth_clear(render_pipeline_t* pipeline, render_clear_action_t action, vector_t color);

RENDER_API void
render_pipeline_flush(render_pipeline_t* pipeline);

#if 0
void
render_pipeline_execute(render_pipeline_t* pipeline);

void
render_pipeline_dispatch(render_pipeline_t* pipeline);

void
render_pipeline_step_initialize(render_pipeline_step_t* step, render_target_t* target,
                                render_pipeline_execute_fn executor);

void
render_pipeline_step_finalize(render_pipeline_step_t* step);

void
render_pipeline_step_blit_initialize(render_pipeline_step_t* step, render_target_t* target_source,
                                     render_target_t* target_destination);
#endif
