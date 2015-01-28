/* target.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file target.h
    Render target */

#include <foundation/platform.h>

#include <render/types.h>


RENDER_API object_t              render_target_create( render_backend_t* backend );
RENDER_API object_t              render_target_ref( object_t target );
RENDER_API void                  render_target_destroy( object_t target );

RENDER_API unsigned int          render_target_width( object_t target );
RENDER_API unsigned int          render_target_height( object_t target );
RENDER_API pixelformat_t         render_target_pixelformat( object_t target );
RENDER_API colorspace_t          render_target_colorspace( object_t target );

