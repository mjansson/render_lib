/* event.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

/*! \file event.h
    Render library event handling */

#include <foundation/platform.h>

#include <render/types.h>

/*! Handle resource events. No other event types should be
passed to this function.
\param event Resource event */
RENDER_API void
render_event_handle_resource(const event_t* event);
