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


/*! Initialize render library
    \return                                             0 if success, <0 if error */
RENDER_API int                                          render_initialize( void );

//! Shutdown render library
RENDER_API void                                         render_shutdown( void );

/*! Query if render library is initialized
    \return                                             true if library is initialized, false if not */
RENDER_API bool                                         render_is_initialized( void );

/*! Enable use of the given APIs
    \param num                                          Number of elements in array
    \param api                                          Array of API identifiers to enable */
RENDER_API void                                         render_api_enable( unsigned int num, render_api_t* api );

/*! Enable use of the given APIs
    \param num                                          Number of elements in array
    \param api                                          Array of API identifiers to disable */
RENDER_API void                                         render_api_disable( unsigned int num, render_api_t* api );
