/* program.h  -  Render library  -  Public Domain  -  2015 Mattias Jansson / Rampant Pixels
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

/*! \file program.h
    Programmable render pipeline program */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_program_t*
render_program_allocate(size_t num_parameters);

RENDER_API void
render_program_initialize(render_program_t* program, size_t num_parameters);

RENDER_API void
render_program_finalize(render_program_t* program);

RENDER_API void
render_program_deallocate(render_program_t* program);

/*! Load the program identified by the given UUID. Returns a pointer
to the existing program if it is already loaded. When loading a program,
the reference count will be used and you must call render_program_unload
to release it, NOT the render_program_deallocate function.
\param backend Backend
\param uuid Program UUID
\return Program */
RENDER_API render_program_t*
render_program_load(render_backend_t* backend, const uuid_t uuid);

/*! Lookup the program identified by the given UUID. When looking up
a program, the reference count will be increased and you must remember to
call render_program_unload to release it.
\param backend Backend
\param uuid Program UUID
\return Program */
RENDER_API render_program_t*
render_program_lookup(render_backend_t* backend, const uuid_t uuid);

RENDER_API bool
render_program_reload(render_program_t* program, const uuid_t uuid);

RENDER_API void
render_program_unload(render_program_t* program);

#define RENDER_PROGRAM_RESOURCE_VERSION 4

#if RESOURCE_ENABLE_LOCAL_SOURCE

/* Compile program resource
\param uuid Program UUID
\param platform Resource platform
\param source Program resource source representation
\param type Type string
\param type_length Length of type string
\return 0 if successful, <0 if error */
RENDER_API int
render_program_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
                       const uint256_t source_hash, const char* type, size_t type_length);

#else

#define render_program_compile(uuid, platform, source, source_hash, type, type_length) ((void)sizeof(uuid)), ((void)sizeof(platform)), ((void)sizeof(source)), ((void)sizeof(source_hash)), ((void)sizeof(type)), ((void)sizeof(type_length)), -1

#endif
