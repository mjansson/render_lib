/* compile.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson
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

/*! \file compile.h
    Render library resource compilation */

#include <foundation/platform.h>
#include <resource/types.h>

#include <render/types.h>

/* Compile render resource
\param uuid Resource UUID
\param platform Resource platform
\param source Resource source representation
\param type Type string
\param type_length Length of type string
\return 0 if successful, <0 if error */
RENDER_API int
render_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const blake3_hash_t source_hash,
               const char* type, size_t type_length);
