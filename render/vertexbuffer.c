/* indexbuffer.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

render_vertexbuffer_t*
render_vertexbuffer_allocate(render_backend_t* backend, render_usage_t usage, size_t num_vertices,
                             size_t buffer_size, const render_vertex_decl_t* decl, const void* data,
                             size_t data_size) {
	render_vertexbuffer_t* buffer = memory_allocate(HASH_RENDER, sizeof(render_vertexbuffer_t), 0,
	                                                MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	buffer->backend = backend;
	buffer->usage = (uint8_t)usage;
	buffer->buffertype = RENDERBUFFER_VERTEX;
	buffer->policy = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->buffersize = buffer_size;
	semaphore_initialize(&buffer->lock, 1);
	memcpy(&buffer->decl, decl, sizeof(render_vertex_decl_t));
	memset(buffer->backend_data, 0, sizeof(buffer->backend_data));

	if (num_vertices) {
		buffer->allocated = num_vertices;
		buffer->used = num_vertices;
		buffer->store = backend->vtable.allocate_buffer(backend, (render_buffer_t*)buffer);
		if (data) {
			if (data_size > buffer->buffersize)
				data_size = buffer->buffersize;
			memcpy(buffer->store, data, data_size);
			buffer->flags |= RENDERBUFFER_DIRTY;
		}
	}

	return buffer;
}

void
render_vertexbuffer_deallocate(render_vertexbuffer_t* buffer) {
	render_buffer_deallocate((render_buffer_t*)buffer);
}

void
render_vertexbuffer_lock(render_vertexbuffer_t* buffer, unsigned int lock) {
	render_buffer_lock((render_buffer_t*)buffer, lock);
}

void
render_vertexbuffer_unlock(render_vertexbuffer_t* buffer) {
	render_buffer_unlock((render_buffer_t*)buffer);
}

void
render_vertexbuffer_upload(render_vertexbuffer_t* buffer) {
	render_buffer_upload((render_buffer_t*)buffer);
}

void
render_vertexbuffer_free(render_vertexbuffer_t* buffer, bool sys, bool aux) {
	buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_vertexbuffer_restore(render_vertexbuffer_t* buffer) {
	buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

	//...
	// All loadable resources should have a stream identifier, an offset and a size
	// to be able to repoen the stream and read the raw buffer back
	//...

	buffer->flags |= RENDERBUFFER_DIRTY;
}

static const uint16_t _vertex_format_size[VERTEXFORMAT_NUMTYPES + 1] = {

    4,   // VERTEXFORMAT_FLOAT
    8,   // VERTEXFORMAT_FLOAT2
    12,  // VERTEXFORMAT_FLOAT3
    16,  // VERTEXFORMAT_FLOAT4

    4,  // VERTEXFORMAT_UBYTE4
    4,  // VERTEXFORMAT_UBYTE4_SNORM

    2,  // VERTEXFORMAT_SHORT
    4,  // VERTEXFORMAT_SHORT2
    8,  // VERTEXFORMAT_SHORT4,

    4,   // VERTEXFORMAT_INT
    8,   // VERTEXFORMAT_INT2
    16,  // VERTEXFORMAT_INT4

    0};

uint16_t
render_vertex_attribute_size(render_vertex_format_t format) {
	return _vertex_format_size[format];
}

size_t
render_vertex_decl_calculate_size(const render_vertex_decl_t* decl) {
	size_t size = 0;
	for (unsigned int i = 0; i < RENDER_MAX_ATTRIBUTES; ++i) {
		if (decl->attribute[i].format >= VERTEXFORMAT_NUMTYPES)
			continue;
		size_t end = decl->attribute[i].offset + _vertex_format_size[decl->attribute[i].format];
		if (end > size)
			size = end;
	}
	return size;
}

render_vertex_decl_t*
render_vertex_decl_allocate(render_vertex_decl_element_t* elements, size_t num_elements) {
	render_vertex_decl_t* decl =
	    memory_allocate(HASH_RENDER, sizeof(render_vertex_decl_t), 0, MEMORY_PERSISTENT);
	render_vertex_decl_initialize(decl, elements, num_elements);
	return decl;
}

render_vertex_decl_t*
render_vertex_decl_allocate_varg(render_vertex_format_t format,
                                 render_vertex_attribute_id attribute, ...) {
	render_vertex_decl_t* decl =
	    memory_allocate(HASH_RENDER, sizeof(render_vertex_decl_t), 0, MEMORY_PERSISTENT);
	va_list list;
	va_start(list, attribute);
	render_vertex_decl_initialize_vlist(decl, format, attribute, list);
	va_end(list);
	return decl;
}

render_vertex_decl_t*
render_vertex_decl_allocate_vlist(render_vertex_format_t format,
                                  render_vertex_attribute_id attribute, va_list list) {
	render_vertex_decl_t* decl =
	    memory_allocate(HASH_RENDER, sizeof(render_vertex_decl_t), 0, MEMORY_PERSISTENT);
	render_vertex_decl_initialize_vlist(decl, format, attribute, list);
	return decl;
}

void
render_vertex_decl_initialize(render_vertex_decl_t* decl, render_vertex_decl_element_t* elements,
                              size_t num_elements) {
	size_t i;
	unsigned int offset = 0;
	memset(decl, 0, sizeof(render_vertex_decl_t));
	for (i = 0; i < RENDER_MAX_ATTRIBUTES; ++i)
		decl->attribute[i].format = VERTEXFORMAT_UNUSED;

	for (i = 0; i < num_elements; ++i) {
		if (elements[i].attribute < RENDER_MAX_ATTRIBUTES) {
			decl->attribute[elements[i].attribute].format = (uint8_t)elements[i].format;
			decl->attribute[elements[i].attribute].binding = 0;
			decl->attribute[elements[i].attribute].offset = (uint16_t)offset;
			offset += _vertex_format_size[elements[i].format];
		}
	}

	for (i = 0; i < RENDER_MAX_ATTRIBUTES; ++i)
		decl->attribute[i].stride = (uint16_t)offset;
}

void
render_vertex_decl_initialize_varg(render_vertex_decl_t* decl, render_vertex_format_t format,
                                   render_vertex_attribute_id attribute, ...) {
	va_list list;
	va_start(list, attribute);
	render_vertex_decl_initialize_vlist(decl, format, attribute, list);
	va_end(list);
}

void
render_vertex_decl_initialize_vlist(render_vertex_decl_t* decl, render_vertex_format_t format,
                                    render_vertex_attribute_id attribute, va_list list) {
	unsigned int offset = 0;
	memset(decl, 0, sizeof(render_vertex_decl_t));
	for (size_t i = 0; i < RENDER_MAX_ATTRIBUTES; ++i)
		decl->attribute[i].format = VERTEXFORMAT_UNUSED;

	va_list clist;
	va_copy(clist, list);

	while (format < VERTEXFORMAT_UNUSED) {
		if (attribute < RENDER_MAX_ATTRIBUTES) {
			decl->attribute[attribute].format = (uint8_t)format;
			decl->attribute[attribute].binding = 0;
			decl->attribute[attribute].offset = (uint16_t)offset;

			offset += _vertex_format_size[format];
		}

		format = va_arg(clist, render_vertex_format_t);
		if (format < VERTEXFORMAT_NUMTYPES)
			attribute = va_arg(clist, render_vertex_attribute_id);
	}

	va_end(clist);

	for (size_t i = 0; i < RENDER_MAX_ATTRIBUTES; ++i)
		decl->attribute[i].stride = (uint16_t)offset;
}

void
render_vertex_decl_finalize(render_vertex_decl_t* decl) {
	FOUNDATION_UNUSED(decl);
}

void
render_vertex_decl_deallocate(render_vertex_decl_t* decl) {
	render_vertex_decl_finalize(decl);
	memory_deallocate(decl);
}
