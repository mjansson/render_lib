/* texture.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file texture.h
    Render texture */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_texture_t*
render_texture_allocate(void);

RENDER_API void
render_texture_initialize(render_texture_t* texture);

RENDER_API void
render_texture_finalize(render_texture_t* texture);

RENDER_API void
render_texture_deallocate(render_texture_t* texture);

/*! Load the texture identified by the given UUID. Returns a pointer
to the existing texture if it is already loaded. When loading a texture,
the reference count will be used and you must call render_texture_unload
to release it, NOT the render_texture_deallocate function.
\param backend Backend
\param uuid Texture UUID
\return Texture */
RENDER_API render_texture_t*
render_texture_load(render_backend_t* backend, const uuid_t uuid);

/*! Lookup the texture identified by the given UUID. When looking up
a texture, the reference count will be used and you must remember to
call render_texture_unload to release it, NOT render_texture_deallocate.
\param backend Backend
\param uuid Shader UUID
\return Shader */
RENDER_API render_texture_t*
render_texture_lookup(render_backend_t* backend, const uuid_t uuid);

RENDER_API bool
render_texture_reload(render_texture_t* texture, const uuid_t uuid);

RENDER_API void
render_texture_unload(render_texture_t* texture);

#define RENDER_TEXTURE_RESOURCE_VERSION 1

#if RESOURCE_ENABLE_LOCAL_SOURCE

/* Compile texture resource
\param uuid Texture UUID
\param platform Resource platform
\param source Texture resource source representation
\param type Type string
\param type_length Length of type string
\return 0 if successful, <0 if error */
RENDER_API int
render_texture_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const uint256_t source_hash,
                       const char* type, size_t type_length);

#else

#define render_texture_compile(uuid, platform, source, source_hash, type, type_length)                    \
	(((void)sizeof(uuid)), ((void)sizeof(platform)), ((void)sizeof(source)), ((void)sizeof(source_hash)), \
	 ((void)sizeof(type)), ((void)sizeof(type_length)), -1)

#endif
