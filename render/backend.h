/* backend.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file backend.h
    Render backend */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_backend_t*
render_backend_allocate(render_api_t api, bool allow_fallback);

RENDER_API void
render_backend_deallocate(render_backend_t* backend);

RENDER_API render_api_t
render_backend_api(render_backend_t* backend);

RENDER_API size_t
render_backend_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity);

RENDER_API size_t
render_backend_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store,
                               size_t capacity);

RENDER_API uint64_t
render_backend_frame_count(render_backend_t* backend);

RENDER_API render_backend_t*
render_backend_thread(void);

RENDER_API uint64_t
render_backend_resource_platform(render_backend_t* backend);

RENDER_API void
render_backend_set_resource_platform(render_backend_t* backend, uint64_t platform);

RENDER_API render_backend_t**
render_backends(void);

RENDER_API bool
render_backend_shader_upload(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size);

RENDER_API void
render_backend_shader_finalize(render_backend_t* backend, render_shader_t* shader);

#define render_backend_shader_table(backend) ((uuidmap_t*)&((backend)->shader_table))
