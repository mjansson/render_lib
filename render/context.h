/* context.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file context.h
    Render context */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_context_t*     
render_context_allocate( size_t commmandsize );

RENDER_API void                  
render_context_deallocate( render_context_t* context );

RENDER_API object_t              
render_context_target( render_context_t* context );

RENDER_API void                  
render_context_set_target( render_context_t* context, object_t target );

RENDER_API render_command_t*     
render_context_reserve( render_context_t* context, uint64_t sort );

RENDER_API void                  
render_context_queue( render_context_t* context, render_command_t* command, uint64_t sort );

RENDER_API size_t          
render_context_reserved( render_context_t* context );

RENDER_API uint8_t               
render_context_group( render_context_t* context );

RENDER_API void                  
render_context_set_group( render_context_t* context, uint8_t group );
