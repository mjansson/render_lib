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

RENDER_API size_t
render_backend_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity);

RENDER_API size_t
render_backend_enumerate_modes(render_backend_t* backend, unsigned int adapter,
                               render_resolution_t* store, size_t capacity);

RENDER_API void
render_backend_set_format(render_backend_t* backend, const pixelformat_t format,
                          const colorspace_t space);

RENDER_API bool
render_backend_set_drawable(render_backend_t* backend, const render_drawable_t* drawable);

RENDER_API render_drawable_t*
render_backend_drawable(render_backend_t* backend);

RENDER_API render_target_t*
render_backend_target_framebuffer(render_backend_t* backend);

RENDER_API void
render_backend_dispatch(render_backend_t* backend, render_target_t* target,
                        render_context_t** contexts, size_t num_contexts);

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

RENDER_API bool
render_backend_shader_upload(render_backend_t* backend, render_shader_t* shader,
                             const void* buffer, size_t size);

RENDER_API uuidmap_t*
render_backend_shader_table(render_backend_t* backend);

RENDER_API bool
render_backend_program_upload(render_backend_t* backend, render_program_t* program);

RENDER_API uuidmap_t*
render_backend_program_table(render_backend_t* backend);

RENDER_API bool
render_backend_texture_upload(render_backend_t* backend, render_texture_t* texture,
                              const void* buffer, size_t size);

RENDER_API void
render_backend_parameter_bind_texture(render_backend_t* backend, void* buffer,
                                      render_texture_t* texture);

RENDER_API void
render_backend_parameter_bind_target(render_backend_t* backend, void* buffer,
                                     render_target_t* target);

RENDER_API uuidmap_t*
render_backend_texture_table(render_backend_t* backend);

RENDER_API render_backend_t**
render_backends(void);
