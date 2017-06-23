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

/*! Get vertex attribute size from format
\param format Vertex attribute format
\return Vertex attribute size */
RENDER_API uint16_t
render_vertex_attribute_size(render_vertex_format_t format);

/*! Allocate a new vertex declaration from array of elements
\param elements Vertex declaration elements
\param num Number of vertex declaration elements in array
\return New allocated vertex declaration */
RENDER_API render_vertex_decl_t*
render_vertex_decl_allocate(render_vertex_decl_element_t* elements, size_t num);

/*! Allocate a new vertex declaration from variable number of elements.
Argument list must be pairs of (render_vertex_format_t, render_vertex_attribute_id)
arguments. Terminate argument list with a VERTEXFORMAT_UNKNOWN format identifier.
\param format Element format identifier
\param attribute Element attribute identifier
\return New allocated vertex declaration */
RENDER_API render_vertex_decl_t*
render_vertex_decl_allocate_varg(render_vertex_format_t format,
                                 render_vertex_attribute_id attribute, ...);

/*! Allocate a new vertex declaration from variable number of elements.
Argument list must be pairs of (render_vertex_format_t, render_vertex_attribute_id)
arguments. Terminate argument list with a VERTEXFORMAT_UNKNOWN format identifier.
\param format Element format identifier
\param attribute Element attribute identifier
\param list Variable argument list
\return New allocated vertex declaration */
RENDER_API render_vertex_decl_t*
render_vertex_decl_allocate_vlist(render_vertex_format_t format,
                                  render_vertex_attribute_id attribute, va_list list);

/*! Initialize a vertex declaration from array of elements
\param decl Vertex declaration
\param elements Vertex declaration elements
\param num Number of vertex declaration elements in array */
RENDER_API void
render_vertex_decl_initialize(render_vertex_decl_t* decl, render_vertex_decl_element_t* elements,
                              size_t num);

/*! Initialize a vertex declaration from variable number of elements.
Argument list must be pairs of (render_vertex_format_t, render_vertex_attribute_id)
arguments. Terminate argument list with a VERTEXFORMAT_UNKNOWN format identifier.
\param decl Vertex declaration
\param format Element format identifier
\param attribute Element attribute identifier */
RENDER_API void
render_vertex_decl_initialize_varg(render_vertex_decl_t* decl, render_vertex_format_t format,
                                   render_vertex_attribute_id attribute, ...);

/*! Initialize a vertex declaration from variable number of elements.
Argument list must be pairs of (render_vertex_format_t, render_vertex_attribute_id)
arguments. Terminate argument list with a VERTEXFORMAT_UNKNOWN format identifier.
\param decl Vertex declaration
\param format Element format identifier
\param attribute Element attribute identifier
\param list Variable argument list */
RENDER_API void
render_vertex_decl_initialize_vlist(render_vertex_decl_t* decl, render_vertex_format_t format,
                                    render_vertex_attribute_id attribute, va_list list);

/*! Finalize a vertex declaration
\param decl Vertex declaration */
RENDER_API void
render_vertex_decl_finalize(render_vertex_decl_t* decl);

/*! Deallocate a vertex declaration
\param decl Vertex declaration */
RENDER_API void
render_vertex_decl_deallocate(render_vertex_decl_t* decl);

/*! Calculate vertex size from declaration
\param decl Vertex declaration
\return Vertex size */
RENDER_API size_t
render_vertex_decl_calculate_size(const render_vertex_decl_t* decl);


RENDER_API object_t
render_vertexbuffer_create(render_backend_t* backend, render_usage_t usage, size_t vertices,
                           const render_vertex_decl_t* decl, const void* data);

RENDER_API object_t
render_vertexbuffer_ref(object_t buffer);

RENDER_API void
render_vertexbuffer_unref(object_t buffer);

RENDER_API render_usage_t
render_vertexbuffer_usage(object_t buffer);

RENDER_API const render_vertex_decl_t*
render_vertexbuffer_decl(object_t buffer);

RENDER_API size_t
render_vertexbuffer_num_allocated(object_t buffer);

RENDER_API size_t
render_vertexbuffer_num_elements(object_t buffer);

RENDER_API void
render_vertexbuffer_set_num_elements(object_t buffer, size_t num);

RENDER_API void
render_vertexbuffer_lock(object_t buffer, unsigned int lock);

RENDER_API void
render_vertexbuffer_unlock(object_t buffer);

RENDER_API render_buffer_uploadpolicy_t
render_vertexbuffer_upload_policy(object_t buffer);

RENDER_API void
render_vertexbuffer_set_upload_policy(object_t buffer, render_buffer_uploadpolicy_t policy);

RENDER_API void
render_vertexbuffer_upload(object_t buffer);

RENDER_API void*
render_vertexbuffer_element(object_t buffer, size_t element);

RENDER_API size_t
render_vertexbuffer_element_size(object_t buffer);

RENDER_API void
render_vertexbuffer_release(object_t buffer, bool sys, bool aux);

RENDER_API void
render_vertexbuffer_restore(object_t buffer);
