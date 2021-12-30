/* backend.h  -  Render library  -  Public Domain  -  2021 Mattias Jansson
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

/*! \file null/backend.h
    Null render backend */

#include <foundation/platform.h>
#include <render/types.h>

#if FOUNDATION_PLATFORM_APPLE

RENDER_API render_backend_t*
render_backend_metal_allocate(void);

#endif
