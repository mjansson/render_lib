/* parameter.h  -  Render library  -  Public Domain  -  2015 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#pragma once

/*! \file parameter.h
    Programmable render pipeline parameters */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API void
render_parameterbuffer_initialize(render_parameterbuffer_t* parameterbuffer, render_backend_t* backend,
                                  render_usage_t usage, const render_parameter_t* parameters, size_t parameters_count,
                                  const void* data, size_t data_size);

RENDER_API render_parameterbuffer_t*
render_parameterbuffer_allocate(render_backend_t* backend, render_usage_t usage, const render_parameter_t* parameters,
                                size_t parameters_count, const void* data, size_t data_size);

RENDER_API void
render_parameterbuffer_deallocate(render_parameterbuffer_t* buffer);

RENDER_API void
render_parameterbuffer_finalize(render_parameterbuffer_t* buffer);

RENDER_API void
render_parameterbuffer_link(render_parameterbuffer_t* buffer, render_program_t* program);

RENDER_API void
render_parameterbuffer_lock(render_parameterbuffer_t* buffer, unsigned int lock);

RENDER_API void
render_parameterbuffer_unlock(render_parameterbuffer_t* buffer);

RENDER_API void
render_parameterbuffer_upload(render_parameterbuffer_t* buffer);

RENDER_API void
render_parameterbuffer_free(render_parameterbuffer_t* buffer, bool sys, bool aux);

RENDER_API void
render_parameterbuffer_restore(render_parameterbuffer_t* buffer);
