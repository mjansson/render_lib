/* shader.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file shader.h
    Programmable render pipeline shader */

#include <foundation/platform.h>

#include <render/types.h>


RENDER_API object_t                        render_pixelshader_create( render_backend_t* backend );
RENDER_API object_t                        render_vertexshader_create( render_backend_t* backend );

RENDER_API object_t                        render_shader_load( const uuid_t uuid );
RENDER_API void                            render_shader_ref( object_t shader );

RENDER_API void                            render_shader_destroy( object_t shader );

RENDER_API void                            render_shader_upload( object_t shader, const void* buffer, unsigned int size );

RENDER_API uuid_t                          render_shader_uuid( object_t shader );
RENDER_API void                            render_shader_set_uuid( object_t shader, const uuid_t uuid );

#endif
