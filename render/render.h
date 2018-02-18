/* render.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file render.h
    Render library entry points */

#include <foundation/platform.h>

#include <render/types.h>
#include <render/hashstrings.h>
#include <render/backend.h>
#include <render/context.h>
#include <render/drawable.h>
#include <render/target.h>
#include <render/command.h>
#include <render/sort.h>
#include <render/indexbuffer.h>
#include <render/vertexbuffer.h>
#include <render/parameter.h>
#include <render/shader.h>
#include <render/pipeline.h>
#include <render/program.h>
#include <render/state.h>
#include <render/texture.h>

#include <render/import.h>
#include <render/compile.h>

/*! Initialize render library
    \return 0 if success, <0 if error */
RENDER_API int
render_module_initialize(render_config_t config);

//! Finalize render library
RENDER_API void
render_module_finalize(void);

/*! Query if render library is initialized
    \return true if library is initialized, false if not */
RENDER_API bool
render_module_is_initialized(void);

/*! Query version of render library
    \return Library version */
RENDER_API version_t
render_module_version(void);

/*! Enable use of the given APIs
    \param api Array of API identifiers to enable
    \param num Number of elements in array */
RENDER_API void
render_api_enable(const render_api_t* api, size_t num);

/*! Enable use of the given APIs
    \param api Array of API identifiers to disable
    \param num Number of elements in array */
RENDER_API void
render_api_disable(const render_api_t* api, size_t num);

/*! Parse config declarations from JSON buffer
\param buffer Data buffer
\param size Size of data buffer
\param tokens JSON tokens
\param num_tokens Number of JSON tokens */
RENDER_API void
render_module_parse_config(const char* path, size_t path_size,
                           const char* buffer, size_t size,
                           const json_token_t* tokens, size_t num_tokens);
