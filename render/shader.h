/* shader.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file shader.h
    Programmable render pipeline shader */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_shader_t*
render_shader_allocate(void);

RENDER_API void
render_shader_initialize(render_shader_t* shader);

RENDER_API void
render_shader_finalize(render_shader_t* shader);

RENDER_API void
render_shader_deallocate(render_shader_t* shader);

/*! Load the shader identified by the given UUID. Returns a pointer
to the existing shader if it is already loaded. When loading a shader,
the reference count will be used and you must call render_shader_unload
to release it, NOT the render_shader_deallocate function.
\param backend Backend
\param uuid Shader UUID
\return Shader */
RENDER_API render_shader_t*
render_shader_load(render_backend_t* backend, const uuid_t uuid);

/*! Lookup the shader identified by the given UUID. When looking up
a shader, the reference count will be used and you must remember to
call render_shader_unload to release it.
\param backend Backend
\param uuid Shader UUID
\return Shader */
RENDER_API render_shader_t*
render_shader_lookup(render_backend_t* backend, const uuid_t uuid);

RENDER_API bool
render_shader_reload(render_shader_t* shader, const uuid_t uuid);

RENDER_API void
render_shader_unload(render_shader_t* shader);

#define RENDER_SHADER_RESOURCE_VERSION 4

#if RESOURCE_ENABLE_LOCAL_SOURCE

/* Compile shader resource
\param uuid Shader UUID
\param platform Resource platform
\param source Shader resource source representation
\param type Type string
\param type_length Length of type string
\return 0 if successful, <0 if error */
RENDER_API int
render_shader_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const blake3_hash_t source_hash,
                      const char* type, size_t type_length);

#else

#define render_shader_compile(uuid, platform, source, source_hash, type, type_length)                     \
	(((void)sizeof(uuid)), ((void)sizeof(platform)), ((void)sizeof(source)), ((void)sizeof(source_hash)), \
	 ((void)sizeof(type)), ((void)sizeof(type_length)), -1)

#endif
