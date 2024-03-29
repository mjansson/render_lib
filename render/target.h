/* target.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson
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

/*! \file target.h
    Render target */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_target_t*
render_target_window_allocate(render_backend_t* backend, window_t* window, uint tag);

RENDER_API render_target_t*
render_target_texture_allocate(render_backend_t* backend, uint width, uint height, render_pixelformat_t format);

RENDER_API void
render_target_deallocate(render_target_t* target);
