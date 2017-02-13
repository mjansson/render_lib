/* backend.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

RENDER_API unsigned int*
render_backend_enumerate_adapters(render_backend_t* backend);

RENDER_API render_resolution_t*
render_backend_enumerate_modes(render_backend_t* backend, unsigned int adapter);

RENDER_API void
render_backend_set_format(render_backend_t* backend, const pixelformat_t format,
                          const colorspace_t space);

RENDER_API void
render_backend_set_drawable(render_backend_t* backend, render_drawable_t* drawable);

RENDER_API render_drawable_t*
render_backend_drawable(render_backend_t* backend);

RENDER_API object_t
render_backend_target_framebuffer(render_backend_t* backend);

RENDER_API void
render_backend_dispatch(render_backend_t* backend, render_context_t** contexts,
                        size_t num_contexts);

RENDER_API void
render_backend_flip(render_backend_t* backend);

RENDER_API uint64_t
render_backend_frame_count(render_backend_t* backend);

RENDER_API void
render_backend_enable_thread(render_backend_t* backend);

RENDER_API void
render_backend_disable_thread(render_backend_t* backend);

RENDER_API render_backend_t*
render_backend_thread(void);

RENDER_API uint64_t
render_backend_resource_platform(render_backend_t* backend);

RENDER_API void
render_backend_set_resource_platform(render_backend_t* backend, uint64_t platform);


RENDER_API object_t
render_backend_shader_acquire(render_backend_t* backend, const uuid_t uuid);

RENDER_API render_shader_t*
render_backend_shader_resolve(render_backend_t* backend, object_t shader);

RENDER_API object_t
render_backend_shader_store(render_backend_t* backend, const uuid_t uuid, render_shader_t* shader);

RENDER_API void
render_backend_shader_release(render_backend_t* backend, object_t shader);


RENDER_API object_t
render_backend_program_acquire(render_backend_t* backend, const uuid_t uuid);

RENDER_API render_program_t*
render_backend_program_resolve(render_backend_t* backend, object_t program);

RENDER_API object_t
render_backend_program_store(render_backend_t* backend, const uuid_t uuid, render_program_t* program);

RENDER_API void
render_backend_program_release(render_backend_t* backend, object_t program);
