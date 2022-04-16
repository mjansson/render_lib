/* buffer.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file buffer.h
    Buffer storing arbitrary data on CPU and GPU accessible memory */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_buffer_t*
render_buffer_allocate(render_backend_t* backend, uint usage, size_t buffer_size, const void* data, size_t data_size);

RENDER_API void
render_buffer_deallocate(render_buffer_t* buffer);

RENDER_API void
render_buffer_lock(render_buffer_t* buffer, unsigned int lock);

RENDER_API void
render_buffer_unlock(render_buffer_t* buffer);

RENDER_API void
render_buffer_upload(render_buffer_t* buffer);

RENDER_API void
render_buffer_free(render_buffer_t* buffer, bool sys, bool aux);

RENDER_API void
render_buffer_restore(render_buffer_t* buffer);

RENDER_API void
render_buffer_data_declare(render_buffer_t* buffer, const render_buffer_data_t* data, size_t data_count,
                           size_t instance_count);

RENDER_API void
render_buffer_data_encode_buffer(render_buffer_t* buffer, uint instance, uint index, render_buffer_t* source,
                                 uint offset);

RENDER_API void
render_buffer_data_encode_matrix(render_buffer_t* buffer, uint instance, uint index, const matrix_t* data);

RENDER_API void
render_buffer_data_encode_constant(render_buffer_t* buffer, uint instance, uint index, const void* data, uint size);
