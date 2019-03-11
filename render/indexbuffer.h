/* indexbuffer.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file indexbuffer.h
    Index buffer storing 16 bit indices */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_indexbuffer_t*
render_indexbuffer_allocate(render_backend_t* backend, render_usage_t type, size_t indices,
                            render_index_format_t format, const void* data);

RENDER_API void
render_indexbuffer_deallocate(render_indexbuffer_t* buffer);

RENDER_API render_indexbuffer_t*
render_indexbuffer_load(const uuid_t uuid);

RENDER_API void
render_indexbuffer_lock(render_indexbuffer_t* buffer, unsigned int lock);

RENDER_API void
render_indexbuffer_unlock(render_indexbuffer_t* buffer);

RENDER_API void
render_indexbuffer_upload(render_indexbuffer_t* buffer);

RENDER_API void
render_indexbuffer_free(render_indexbuffer_t* buffer, bool sys, bool aux);

RENDER_API void
render_indexbuffer_restore(render_indexbuffer_t* buffer);
