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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

/*! \file state.h
    Render pipeline state */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API object_t
render_statebuffer_create(render_backend_t* backend, render_usage_t usage,
                          const render_state_t state);

RENDER_API object_t
render_statebuffer_ref(object_t buffer);

RENDER_API void
render_statebuffer_destroy(object_t buffer);

RENDER_API void
render_statebuffer_lock(object_t buffer, unsigned int lock);

RENDER_API void
render_statebuffer_unlock(object_t buffer);

RENDER_API render_buffer_uploadpolicy_t
render_statebuffer_upload_policy(object_t buffer);

RENDER_API void
render_statebuffer_set_upload_policy(object_t buffer, render_buffer_uploadpolicy_t policy);

RENDER_API render_state_t*
render_statebuffer_data(object_t buffer);

RENDER_API void
render_statebuffer_upload(object_t buffer);

RENDER_API void
render_statebuffer_release(object_t buffer, bool sys, bool aux);

RENDER_API void
render_statebuffer_restore(object_t buffer);
