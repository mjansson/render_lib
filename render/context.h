/* context.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file context.h
    Render context */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_context_t*
render_context_allocate(size_t commmandsize);

RENDER_API void
render_context_deallocate(render_context_t* context);

RENDER_API render_command_t*
render_context_reserve(render_context_t* context, uint64_t sort);

RENDER_API void
render_context_queue(render_context_t* context, render_command_t* command, uint64_t sort);

RENDER_API size_t
render_context_reserved(render_context_t* context);
