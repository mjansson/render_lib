/* sort.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

/*! \file sort.h
    Render context */

#include <foundation/platform.h>

#include <render/types.h>


RENDER_API void             render_sort_merge( render_context_t** contexts, unsigned int num_contexts );
RENDER_API void             render_sort_reset( render_context_t* context );
RENDER_API uint64_t         render_sort_sequential_key( render_context_t* context );
RENDER_API uint64_t         render_sort_render_key( render_context_t* context, uint64_t vertexbuffer, uint64_t indexbuffer, uint64_t blend_state );
