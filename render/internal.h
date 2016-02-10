/* internal.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file internal.h
    Internal types */

#include <foundation/platform.h>
#include <foundation/types.h>
#include <foundation/internal.h>

#include <window/types.h>
#include <resource/types.h>

#include <render/types.h>
#include <render/hashstrings.h>

RENDER_EXTERN bool         _render_api_disabled[];
RENDER_EXTERN objectmap_t* _render_map_target;
RENDER_EXTERN objectmap_t* _render_map_buffer;
RENDER_EXTERN render_config_t _render_config;

// INTERNAL FUNCTIONS
RENDER_EXTERN int
render_target_initialize(void);

RENDER_EXTERN void
render_target_finalize(void);

RENDER_EXTERN object_t
render_target_create_framebuffer(render_backend_t* backend);

RENDER_EXTERN render_target_t*
render_target_lookup(object_t id);

RENDER_EXTERN int
render_buffer_initialize(void);

RENDER_EXTERN void
render_buffer_finalize(void);

RENDER_EXTERN object_t
render_buffer_ref(object_t buffer);

RENDER_EXTERN void
render_buffer_destroy(object_t id);

RENDER_EXTERN void
render_buffer_upload(render_buffer_t* buffer);

RENDER_EXTERN void
render_buffer_lock(object_t id, unsigned int lock);

RENDER_EXTERN void
render_buffer_unlock(object_t id);
