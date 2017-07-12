/* shader.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file shader.h
    Programmable render pipeline shader */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_shader_t*
render_pixelshader_allocate(void);

RENDER_API void
render_pixelshader_initialize(render_shader_t* shader);

RENDER_API render_shader_t*
render_vertexshader_allocate(void);

RENDER_API void
render_vertexshader_initialize(render_shader_t* shader);


RENDER_API void
render_shader_finalize(render_shader_t* shader);

RENDER_API void
render_shader_deallocate(render_shader_t* shader);

RENDER_API bool
render_shader_reload(render_shader_t* shader, const uuid_t uuid);

RENDER_API bool
render_shader_reload(render_shader_t* shader, const uuid_t uuid);

RENDER_API object_t
render_shader_load(render_backend_t* backend, const uuid_t uuid);


#define RENDER_SHADER_RESOURCE_VERSION 2

#if RESOURCE_ENABLE_LOCAL_SOURCE

/* Compile shader resource
\param uuid Shader UUID
\param platform Resource platform
\param source Shader resource source representation
\param type Type string
\param type_length Length of type string
\return 0 if successful, <0 if error */
RENDER_API int
render_shader_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
                      const uint256_t source_hash, const char* type, size_t type_length);

#else

#define render_shader_compile(uuid, platform, source, source_hash, type, type_length) (((void)sizeof(uuid)), ((void)sizeof(platform)), ((void)sizeof(source)), ((void)sizeof(source_hash)), ((void)sizeof(type)), ((void)sizeof(type_length)), -1)

#endif
