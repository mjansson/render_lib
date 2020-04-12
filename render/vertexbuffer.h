/* vertexbuffer.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/render_lib
 *
 * The dependent library source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson
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
\param elements_count Number of vertex declaration elements in array
\return New allocated vertex declaration */
RENDER_API render_vertex_decl_t*
render_vertex_decl_allocate(render_vertex_decl_element_t* elements, size_t elements_count);

/*! Allocate a new vertex declaration from variable number of elements.
Argument list must be pairs of (render_vertex_format_t, render_vertex_attribute_id)
arguments. Terminate argument list with a VERTEXFORMAT_UNKNOWN format identifier.
\param format Element format identifier
\param attribute Element attribute identifier
\return New allocated vertex declaration */
RENDER_API render_vertex_decl_t*
render_vertex_decl_allocate_varg(render_vertex_format_t format, render_vertex_attribute_id attribute, ...);

/*! Allocate a new vertex declaration from variable number of elements.
Argument list must be pairs of (render_vertex_format_t, render_vertex_attribute_id)
arguments. Terminate argument list with a VERTEXFORMAT_UNKNOWN format identifier.
\param format Element format identifier
\param attribute Element attribute identifier
\param list Variable argument list
\return New allocated vertex declaration */
RENDER_API render_vertex_decl_t*
render_vertex_decl_allocate_vlist(render_vertex_format_t format, render_vertex_attribute_id attribute, va_list list);

/*! Initialize a vertex declaration from array of elements
\param decl Vertex declaration
\param elements Vertex declaration elements
\param elements_count Number of vertex declaration elements in array */
RENDER_API void
render_vertex_decl_initialize(render_vertex_decl_t* decl, render_vertex_decl_element_t* elements,
                              size_t elements_count);

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

RENDER_API render_vertexbuffer_t*
render_vertexbuffer_allocate(render_backend_t* backend, render_usage_t usage, size_t vertex_count, size_t buffer_size,
                             const render_vertex_decl_t* decl, const void* data, size_t data_size);

RENDER_API void
render_vertexbuffer_deallocate(render_vertexbuffer_t* buffer);

RENDER_API void
render_vertexbuffer_lock(render_vertexbuffer_t* buffer, unsigned int lock);

RENDER_API void
render_vertexbuffer_unlock(render_vertexbuffer_t* buffer);

RENDER_API void
render_vertexbuffer_upload(render_vertexbuffer_t* buffer);

RENDER_API void
render_vertexbuffer_free(render_vertexbuffer_t* buffer, bool sys, bool aux);

RENDER_API void
render_vertexbuffer_restore(render_vertexbuffer_t* buffer);
