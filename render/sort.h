/* sort.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

/*! \file sort.h
    Sorting of render commands */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API void
render_sort_merge(render_context_t** contexts, size_t contexts_count);

RENDER_API void
render_sort_reset(render_context_t* context);

RENDER_API uint64_t
render_sort_sequential_key(render_context_t* context);

RENDER_API uint64_t
render_sort_render_key(render_context_t* context, render_buffer_t* vertexbuffer, render_buffer_t* indexbuffer,
                       render_state_t* state);
