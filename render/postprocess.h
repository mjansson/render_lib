/* postprocess.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file postprocess.h
    Post processing */

#include <foundation/platform.h>
#include <resource/types.h>

#include <render/types.h>

render_postprocess_t*
render_postprocess_allocate(void);

void
render_postprocess_initialize(render_postprocess_t* postprocess);

void
render_postprocess_finalize(render_postprocess_t* postprocess);

void
render_postprocess_deallocate(render_postprocess_t* postprocess);

void
render_postprocess_process(render_postprocess_t* postprocess);
