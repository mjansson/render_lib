/* import.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file import.h
    Render library resource import */

#include <foundation/platform.h>
#include <resource/types.h>

/* Import render source to a resource
\param stream Source stream
\param uuid Resource UUID
\return 0 if successful, <0 if error */
RENDER_API int
render_import(stream_t* stream, const uuid_t uuid);
