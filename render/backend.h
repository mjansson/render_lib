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

/*! Load the shader identified by the given UUID and return an
object handle to the shader. The shader reference count will have
increased by one (if the shader was previously not loaded the reference
count will be equal to one).
\param backend Backend
\param uuid Shader UUID
\return Shader object handle */
RENDER_API object_t
render_backend_shader_load(render_backend_t* backend, const uuid_t uuid);

/*! Lookup the shader identified by the given UUID and return an
object handle to the shader. The shader reference count will have
increased by one.
\param backend Backend
\param uuid Shader UUID
\return Shader object handle */
RENDER_API object_t
render_backend_shader_lookup(render_backend_t* backend, const uuid_t uuid);

/*! Get the raw shader pointer for the given shader object handle. Does not
increase the reference count, the caller must hold a valid reference to
guarantee the shader lifetime during the use of the returned pointer
\param backend Backend
\param shader Shader object handle
\return Shader pointer */
RENDER_API render_shader_t*
render_backend_shader_raw(render_backend_t* backend, object_t shader);

/*! Increase the shader reference count by one and retrieve the raw shader
pointer for the given shader object handle. Returns null if the shader object
handle was invalid.
\param backend Backend
\param shader Shader object handle
\return Shader pointer */
RENDER_API render_shader_t*
render_backend_shader_acquire(render_backend_t* backend, object_t shader);

/*! Release the shader object handle, decreasing the shader reference
count by one and deallocating the shader if it reaches zero.
\param backend Backend
\param shader Shader object handle */
RENDER_API void
render_backend_shader_release(render_backend_t* backend, object_t shader);

/*! Bind the give shader pointer to the given UUID and return a shader
object handle for the shader. The shader reference count will have
increased by one (if the shader was previously not loaded the reference
count will be equal to one). If the shader was already bound to the given
UUID the reference count will simply be increased.
\param backend Backend
\param uuid Shader UUID
\param shader Shader pointer
\return Shader object handle */
RENDER_API object_t
render_backend_shader_bind(render_backend_t* backend, const uuid_t uuid,
                           render_shader_t* shader);

RENDER_API bool
render_backend_shader_upload(render_backend_t* backend, render_shader_t* shader,
                             const void* buffer, size_t size);

RENDER_API object_t
render_backend_program_load(render_backend_t* backend, const uuid_t uuid);

RENDER_API object_t
render_backend_program_lookup(render_backend_t* backend, const uuid_t uuid);

RENDER_API render_program_t*
render_backend_program_raw(render_backend_t* backend, object_t program);

RENDER_API render_program_t*
render_backend_program_acquire(render_backend_t* backend, object_t program);

RENDER_API void
render_backend_program_release(render_backend_t* backend, object_t program);

RENDER_API object_t
render_backend_program_bind(render_backend_t* backend, const uuid_t uuid,
                            render_program_t* program);

RENDER_API bool
render_backend_program_upload(render_backend_t* backend, render_program_t* program);
