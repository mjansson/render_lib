/* projection.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file projection.h
    Projection matrix */

#include <foundation/platform.h>

#include <render/types.h>
#include <vector/types.h>

RENDER_API matrix_t
render_projection_perspective(real near, real far, real fov, real aspect);

RENDER_API matrix_t
render_projection_orthographic(real near, real far, real left, real top, real right, real bottom);
