/* drawable.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file drawable.h
    Render drawable */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_drawable_t*
render_drawable_allocate(void);

RENDER_API void
render_drawable_initialize_window(render_drawable_t* drawable, window_t* window, unsigned int tag);

RENDER_API void
render_drawable_initialize_fullscreen(render_drawable_t* drawable, unsigned int adapter, unsigned int width,
                                      unsigned int height, unsigned int refresh);

RENDER_API void
render_drawable_finalize(render_drawable_t* drawable);

RENDER_API void
render_drawable_deallocate(render_drawable_t* drawable);
