/* build.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file build.h
    Build setup */

#include <foundation/platform.h>


#if defined( RENDER_COMPILE ) && RENDER_COMPILE
#  ifdef __cplusplus
#  define RENDER_EXTERN extern "C"
#  define RENDER_API extern "C"
#  else
#  define RENDER_EXTERN extern
#  define RENDER_API extern
#  endif
#else
#  ifdef __cplusplus
#  define RENDER_EXTERN extern "C"
#  define RENDER_API extern "C"
#  else
#  define RENDER_EXTERN extern
#  define RENDER_API extern
#  endif
#endif


#define RENDER_ENABLE_NVGLEXPERT                     0


// Allocation sizes

//! Maximum number of concurrently allocated render targets
#define BUILD_SIZE_RENDER_TARGET_MAP                 128

