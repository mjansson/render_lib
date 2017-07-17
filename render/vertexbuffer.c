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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

#define GET_BUFFER(id) objectmap_lookup(_render_map_buffer, (id))

object_t
render_vertexbuffer_create(render_backend_t* backend, render_usage_t usage, size_t vertices,
                           const render_vertex_decl_t* decl, const void* data) {
	object_t id = objectmap_reserve(_render_map_buffer);
	if (!id) {
		log_error(HASH_RENDER, ERROR_OUT_OF_MEMORY,
		          STRING_CONST("Unable to allocate vertex buffer, out of slots in object map"));
		return 0;
	}

	memory_context_push(HASH_RENDER);

	render_vertexbuffer_t* buffer = memory_allocate(HASH_RENDER, sizeof(render_vertexbuffer_t), 0,
	                                                MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	buffer->backend    = backend;
	buffer->usage      = usage;
	buffer->buffertype = RENDERBUFFER_VERTEX;
	buffer->policy     = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->size       = decl->size;
	semaphore_initialize(&buffer->lock, 1);
	memcpy(&buffer->decl, decl, sizeof(render_vertex_decl_t));
	objectmap_set(_render_map_buffer, id, buffer);

	if (vertices) {
		buffer->allocated = vertices;
		buffer->used = vertices;
		buffer->store = backend->vtable.allocate_buffer(backend, (render_buffer_t*)buffer);
		if (data) {
			memcpy(buffer->store, data, vertices * buffer->size);
			buffer->flags |= RENDERBUFFER_DIRTY;
		}
	}

	memory_context_pop();

	return id;
}

render_vertexbuffer_t*
render_vertexbuffer_acquire(object_t id) {
	return (render_vertexbuffer_t*)render_buffer_acquire(id);
}

void
render_vertexbuffer_release(object_t id) {
	render_buffer_release(id);
}

render_usage_t
render_vertexbuffer_usage(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? (render_usage_t)buffer->usage : RENDERUSAGE_INVALID;
}

const render_vertex_decl_t*
render_vertexbuffer_decl(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? &buffer->decl : 0;
}

size_t
render_vertexbuffer_num_allocated(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->allocated : 0;
}

size_t
render_vertexbuffer_num_elements(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->used : 0;
}

void render_vertexbuffer_set_num_elements(object_t id, size_t num) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer) {
		buffer->used = (buffer->allocated < num) ? buffer->allocated : num;
		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}

void
render_vertexbuffer_lock(object_t id, unsigned int lock) {
	render_buffer_lock(id, lock);
}

void
render_vertexbuffer_unlock(object_t id) {
	render_buffer_unlock(id);
}

render_buffer_uploadpolicy_t
render_vertexbuffer_upload_policy(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? (render_buffer_uploadpolicy_t)buffer->policy : RENDERBUFFER_UPLOAD_ONDISPATCH;
}

void
render_vertexbuffer_set_upload_policy(object_t id, render_buffer_uploadpolicy_t policy) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->policy = policy;
}

void
render_vertexbuffer_upload(object_t id) {
	render_buffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		render_buffer_upload(buffer);
}

void*
render_vertexbuffer_element(object_t id, size_t element) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return pointer_offset(buffer->access, buffer->size * element);
}

size_t
render_vertexbuffer_element_size(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	return buffer ? buffer->size : 0;
}

void
render_vertexbuffer_free(object_t id, bool sys, bool aux) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer)
		buffer->backend->vtable.deallocate_buffer(buffer->backend, (render_buffer_t*)buffer, sys, aux);
}

void
render_vertexbuffer_restore(object_t id) {
	render_vertexbuffer_t* buffer = GET_BUFFER(id);
	if (buffer) {
		buffer->backend->vtable.allocate_buffer(buffer->backend, (render_buffer_t*)buffer);

		//...
		//All loadable resources should have a stream identifier, an offset and a size
		//to be able to repoen the stream and read the raw buffer back
		//...

		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}

static const uint16_t _vertex_format_size[ VERTEXFORMAT_UNKNOWN + 1 ] = {

	4,  //VERTEXFORMAT_FLOAT
	8,  //VERTEXFORMAT_FLOAT2
	12, //VERTEXFORMAT_FLOAT3
	16, //VERTEXFORMAT_FLOAT4

	4,  //VERTEXFORMAT_UBYTE4
	4,  //VERTEXFORMAT_UBYTE4_SNORM

	2,  //VERTEXFORMAT_SHORT
	4,  //VERTEXFORMAT_SHORT2
	8,  //VERTEXFORMAT_SHORT4,

	4,  //VERTEXFORMAT_INT
	8,  //VERTEXFORMAT_INT2
	16, //VERTEXFORMAT_INT4

	0,
	0
};

uint16_t
render_vertex_attribute_size(render_vertex_format_t format) {
	return _vertex_format_size[format];
}

size_t
render_vertex_decl_calculate_size(const render_vertex_decl_t* decl) {
	size_t size = 0;
	for (unsigned int i = 0; i < decl->num_attributes; ++i) {
		FOUNDATION_ASSERT_MSGFORMAT(decl->attribute[i].format <= VERTEXFORMAT_UNKNOWN,
		                            "Invalid vertex format type %d index %d", decl->attribute[i].format, i);
		size_t end = decl->attribute[i].offset + _vertex_format_size[ decl->attribute[i].format ];
		if (end > size)
			size = end;
	}
	return size;
}

render_vertex_decl_t*
render_vertex_decl_allocate(render_vertex_decl_element_t* elements, size_t num_elements) {
	render_vertex_decl_t* decl = memory_allocate(HASH_RENDER, sizeof(render_vertex_decl_t), 0,
	                                             MEMORY_PERSISTENT);
	render_vertex_decl_initialize(decl, elements, num_elements);
	return decl;
}

render_vertex_decl_t*
render_vertex_decl_allocate_varg(render_vertex_format_t format,
                                 render_vertex_attribute_id attribute, ...) {
	render_vertex_decl_t* decl = memory_allocate(HASH_RENDER, sizeof(render_vertex_decl_t), 0,
	                                             MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	va_list list;
	va_start(list, attribute);
	render_vertex_decl_initialize_vlist(decl, format, attribute, list);
	va_end(list);
	return decl;
}

render_vertex_decl_t*
render_vertex_decl_allocate_vlist(render_vertex_format_t format,
                                  render_vertex_attribute_id attribute, va_list list) {
	render_vertex_decl_t* decl = memory_allocate(HASH_RENDER, sizeof(render_vertex_decl_t), 0,
	                                             MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	render_vertex_decl_initialize_vlist(decl, format, attribute, list);
	return decl;
}

void
render_vertex_decl_initialize(render_vertex_decl_t* decl, render_vertex_decl_element_t* elements,
                              size_t num_elements) {
	size_t i;
	unsigned int offset = 0;

	if (num_elements > VERTEXATTRIBUTE_NUMATTRIBUTES)
		num_elements = VERTEXATTRIBUTE_NUMATTRIBUTES;

	for (i = 0; i < VERTEXATTRIBUTE_NUMATTRIBUTES; ++i)
		decl->attribute[i].format = VERTEXFORMAT_UNKNOWN;

	for (i = 0; i < num_elements; ++i) {
		decl->attribute[elements[i].attribute].format = elements[i].format;
		decl->attribute[elements[i].attribute].binding = 0;
		decl->attribute[elements[i].attribute].offset = (uint16_t)offset;

		offset += _vertex_format_size[ elements[i].format ];
	}

	decl->num_attributes = (unsigned int)num_elements;
	decl->size = offset;
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

	decl->num_attributes = 0;

	for (size_t i = 0; i < VERTEXATTRIBUTE_NUMATTRIBUTES; ++i)
		decl->attribute[i].format = VERTEXFORMAT_UNKNOWN;

	va_list clist;
	va_copy(clist, list);

	while (format < VERTEXFORMAT_UNKNOWN) {
		if (attribute < VERTEXATTRIBUTE_NUMATTRIBUTES) {
			decl->attribute[attribute].format = format;
			decl->attribute[attribute].binding = 0;
			decl->attribute[attribute].offset = (uint16_t)offset;
			++decl->num_attributes;

			offset += _vertex_format_size[ format ];
		}

		format = va_arg(clist, render_vertex_format_t);
		if (format <= VERTEXFORMAT_UNKNOWN)
			attribute = va_arg(clist, render_vertex_attribute_id);
	}

	va_end(clist);

	decl->size = offset;
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
