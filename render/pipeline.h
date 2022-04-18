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
render_pipeline_allocate(render_backend_t* backend, render_indexformat_t index_format, uint capacity);

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

RENDER_API void
render_pipeline_queue(render_pipeline_t* pipeline, render_primitive_type type, const render_primitive_t* primitive);

RENDER_API void
render_pipeline_use_argument_buffer(render_pipeline_t* pipeline, render_buffer_index_t buffer);

RENDER_API void
render_pipeline_use_render_buffer(render_pipeline_t* pipeline, render_buffer_index_t buffer);

RENDER_API render_pipeline_state_t
render_pipeline_state_allocate(render_backend_t* backend, render_pipeline_t* pipeline, render_shader_t* shader);

RENDER_API void
render_pipeline_state_deallocate(render_backend_t* backend, render_pipeline_state_t state);
