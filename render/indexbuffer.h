/* indexbuffer.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file indexbuffer.h
    Index buffer storing 16 bit indices */

#include <foundation/platform.h>

#include <render/types.h>


RENDER_API object_t                        render_indexbuffer_create( render_backend_t* backend, render_usage_t type, unsigned int indices, const uint16_t* data );
RENDER_API object_t                        render_indexbuffer_load( const uuid_t uuid );
RENDER_API object_t                        render_indexbuffer_ref( object_t buffer );
RENDER_API void                            render_indexbuffer_destroy( object_t buffer );

RENDER_API render_usage_t                  render_indexbuffer_usage( object_t buffer );
RENDER_API unsigned int                    render_indexbuffer_num_allocated( object_t buffer );
RENDER_API unsigned int                    render_indexbuffer_num_elements( object_t buffer );
RENDER_API void                            render_indexbuffer_set_num_elements( object_t buffer, unsigned int num );

RENDER_API void                            render_indexbuffer_lock( object_t buffer, unsigned int lock );
RENDER_API void                            render_indexbuffer_unlock( object_t buffer );

RENDER_API render_buffer_uploadpolicy_t    render_indexbuffer_upload_policy( object_t buffer );
RENDER_API void                            render_indexbuffer_set_upload_policy( object_t buffer, render_buffer_uploadpolicy_t policy );
RENDER_API void                            render_indexbuffer_upload( object_t buffer );

RENDER_API uint16_t*                       render_indexbuffer_element( object_t buffer, unsigned int element );

RENDER_API void                            render_indexbuffer_release( object_t buffer, bool sys, bool aux );
RENDER_API void                            render_indexbuffer_restore( object_t buffer );

RENDER_API uuid_t                          render_indexbuffer_uuid( object_t buffer );
RENDER_API void                            render_indexbuffer_set_uuid( object_t buffer, const uuid_t uuid );
