/* state.h  -  Render library  -  Public Domain  -  2015 Mattias Jansson / Rampant Pixels
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

#pragma once

/*! \file state.h
    Render pipeline state */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_state_t
render_state_default(void);

RENDER_API void
render_statebuffer_initialize(render_statebuffer_t* statebuffer, render_backend_t* backend,
                              render_usage_t usage, const render_state_t state);

RENDER_API render_statebuffer_t*
render_statebuffer_allocate(render_backend_t* backend, render_usage_t usage,
                            const render_state_t state);

RENDER_API void
render_statebuffer_deallocate(render_statebuffer_t* buffer);

RENDER_API void
render_statebuffer_finalize(render_statebuffer_t* buffer);

RENDER_API void
render_statebuffer_lock(render_statebuffer_t* buffer, unsigned int lock);

RENDER_API void
render_statebuffer_unlock(render_statebuffer_t* buffer);

RENDER_API render_state_t*
render_statebuffer_data(render_statebuffer_t* buffer);

RENDER_API void
render_statebuffer_upload(render_statebuffer_t* buffer);

RENDER_API void
render_statebuffer_free(render_statebuffer_t* buffer, bool sys, bool aux);

RENDER_API void
render_statebuffer_restore(render_statebuffer_t* buffer);
