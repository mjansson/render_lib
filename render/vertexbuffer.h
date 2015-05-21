/* vertexbuffer.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file vertexbuffer.h
    Vertex buffer storing configurable format vertices */

#include <foundation/platform.h>

#include <render/types.h>


RENDER_API render_vertex_decl_t*           render_vertex_decl_allocate_buffer( unsigned int num, render_vertex_decl_element_t* elements );
RENDER_API render_vertex_decl_t*           render_vertex_decl_allocate( render_vertex_format_t format, render_vertex_attribute_id attribute, ... );
RENDER_API unsigned int                    render_vertex_decl_size( const render_vertex_decl_t* decl );

RENDER_API object_t                        render_vertexbuffer_create( render_backend_t* backend, render_usage_t usage, unsigned int vertices, const render_vertex_decl_t* decl, const void* data );
RENDER_API object_t                        render_vertexbuffer_load( render_backend_t* backend, const uuid_t uuid );
RENDER_API object_t                        render_vertexbuffer_ref( object_t buffer );
RENDER_API void                            render_vertexbuffer_destroy( object_t buffer );

RENDER_API render_usage_t                  render_vertexbuffer_usage( object_t buffer );
RENDER_API const render_vertex_decl_t*     render_vertexbuffer_decl( object_t buffer );
RENDER_API unsigned int                    render_vertexbuffer_num_allocated( object_t buffer );
RENDER_API unsigned int                    render_vertexbuffer_num_elements( object_t buffer );
RENDER_API void                            render_vertexbuffer_set_num_elements( object_t buffer, unsigned int num );

RENDER_API void                            render_vertexbuffer_lock( object_t buffer, unsigned int lock );
RENDER_API void                            render_vertexbuffer_unlock( object_t buffer );

RENDER_API render_buffer_uploadpolicy_t    render_vertexbuffer_upload_policy( object_t buffer );
RENDER_API void                            render_vertexbuffer_set_upload_policy( object_t buffer, render_buffer_uploadpolicy_t policy );
RENDER_API void                            render_vertexbuffer_upload( object_t buffer );

RENDER_API void*                           render_vertexbuffer_element( object_t buffer, unsigned int element );
RENDER_API unsigned int                    render_vertexbuffer_element_size( object_t buffer );

RENDER_API void                            render_vertexbuffer_release( object_t buffer, bool sys, bool aux );
RENDER_API void                            render_vertexbuffer_restore( object_t buffer );

RENDER_API uuid_t                          render_vertexbuffer_uuid( object_t buffer );
RENDER_API void                            render_vertexbuffer_set_uuid( object_t buffer, const uuid_t uuid );
