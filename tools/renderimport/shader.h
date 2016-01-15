/* shader.h  -  Render library importer  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

#include <foundation/foundation.h>
#include <render/render.h>

#include "main.h"

RENDER_API uuid_t
renderimport_shader_check_referenced_uuid(stream_t* stream);

RENDER_API renderimport_type_t
renderimport_shader_guess_type(stream_t* stream);

RENDER_API int
renderimport_import_shader(stream_t* stream, const uuid_t uuid);
