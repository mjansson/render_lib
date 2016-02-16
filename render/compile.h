/* compile.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file compile.h
    Render library resource compilation */

#include <foundation/platform.h>
#include <resource/types.h>

#include <render/types.h>

#if RESOURCE_ENABLE_LOCAL_SOURCE

/* Compile render resource
\param uuid Resource UUID
\param platform Resource platform
\param source Resource source representation
\param type Type string
\param type_length Length of type string
\return 0 if successful, <0 if error */
RENDER_API int
render_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
               const uint256_t source_hash, const char* type, size_t type_length);

#else

#define render_compile(uuid, platform, source, source_hash, type, type_length) (((void)sizeof(uuid)), ((void)sizeof(platform)), ((void)sizeof(source)), ((void)sizeof(source_hash)), ((void)sizeof(type)), ((void)sizeof(type_length)), -1)

#endif
