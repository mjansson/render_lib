/* errorcodes.h  -  Render library importer  -  Public Domain  -  2015 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform resource I/O library in C11 providing
 * basic resource loading, saving and streaming functionality for projects based
 * on our foundation library.
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

//Error codes returned by renderimport tool
#define RENDERIMPORT_RESULT_OK                           0
#define RENDERIMPORT_RESULT_UNSUPPORTED_INPUT           -1
#define RENDERIMPORT_RESULT_INVALID_ARGUMENT            -2
#define RENDERIMPORT_RESULT_UNKNOWN_COMMAND             -3
#define RENDERIMPORT_RESULT_UNABLE_TO_OPEN_OUTPUT_FILE  -4
#define RENDERIMPORT_RESULT_INVALID_INPUT               -5
