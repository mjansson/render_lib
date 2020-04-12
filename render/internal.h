/* internal.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson
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

/*! \file internal.h
    Internal types */

#include <foundation/platform.h>
#include <foundation/types.h>
#include <foundation/internal.h>

#include <window/types.h>
#include <resource/types.h>

#include <render/types.h>
#include <render/hashstrings.h>

RENDER_EXTERN bool _render_api_disabled[];
RENDER_EXTERN render_config_t _render_config;
RENDER_EXTERN render_backend_t** _render_backends;

// INTERNAL FUNCTIONS

RENDER_EXTERN void
render_target_initialize_framebuffer(render_target_t* target, render_backend_t* backend);

RENDER_EXTERN void
render_buffer_deallocate(render_buffer_t* buffer);

RENDER_EXTERN void
render_buffer_upload(render_buffer_t* buffer);

RENDER_EXTERN void
render_buffer_lock(render_buffer_t* buffer, unsigned int lock);

RENDER_EXTERN void
render_buffer_unlock(render_buffer_t* buffer);

RENDER_EXTERN render_shader_t*
render_shader_load_raw(render_backend_t* backend, const uuid_t uuid);

RENDER_EXTERN render_program_t*
render_program_load_raw(render_backend_t* backend, const uuid_t uuid);

RENDER_EXTERN render_texture_t*
render_texture_load_raw(render_backend_t* backend, const uuid_t uuid);
