/* drawable.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file drawable.h
    Render drawable */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_drawable_t*
render_drawable_allocate();

RENDER_API void
render_drawable_deallocate(render_drawable_t* drawable);

#if FOUNDATION_PLATFORM_IOS
RENDER_API void
render_drawable_set_window(render_drawable_t* drawable, window_t* window, int tag);
#else
RENDER_API void
render_drawable_set_window(render_drawable_t* drawable, window_t* window);
#endif

RENDER_API void
render_drawable_set_offscreen(render_drawable_t* drawable, object_t buffer);

RENDER_API void
render_drawable_set_fullscreen(render_drawable_t* drawable, unsigned int adapter,
                               int width, int height, int refresh);

RENDER_API render_drawable_type_t
render_drawable_type(render_drawable_t* drawable);

RENDER_API int
render_drawable_width(render_drawable_t* drawable);

RENDER_API int
render_drawable_height(render_drawable_t* drawable);
