/* shader.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include <resource/stream.h>
#include <resource/platform.h>
#include <resource/compile.h>

FOUNDATION_STATIC_ASSERT(sizeof(render_shader_t) == 64, "invalid shader size");

render_shader_t*
render_pixelshader_allocate(void) {
	render_shader_t* shader = memory_allocate(HASH_RENDER, sizeof(render_shader_t), 0,
	                                          MEMORY_PERSISTENT);
	render_pixelshader_initialize(shader);
	return shader;
}

void
render_pixelshader_initialize(render_shader_t* shader) {
	memset(shader, 0, sizeof(render_shader_t));
	shader->shadertype = SHADER_PIXEL;
}

render_shader_t*
render_vertexshader_allocate(void) {
	render_shader_t* shader = memory_allocate(HASH_RENDER, sizeof(render_shader_t), 16,
	                                          MEMORY_PERSISTENT);
	render_vertexshader_initialize(shader);
	return shader;
}

void
render_vertexshader_initialize(render_shader_t* shader) {
	memset(shader, 0, sizeof(render_shader_t));
	shader->shadertype = SHADER_VERTEX;
}

void
render_shader_finalize(render_shader_t* shader) {
	if (shader->backend)
		shader->backend->vtable.deallocate_shader(shader->backend, shader);
}

void
render_shader_deallocate(render_shader_t* shader) {
	if (shader)
		render_shader_finalize(shader);
	memory_deallocate(shader);
}

bool
render_shader_upload(render_backend_t* backend, render_shader_t* shader,
                     const void* buffer, size_t size) {
	if (shader->backend && (shader->backend != backend))
		shader->backend->vtable.deallocate_shader(shader->backend, shader);
	if (backend->vtable.upload_shader(backend, shader, buffer, size)) {
		shader->backend = backend;
		return true;
	}
	return false;
}

object_t
render_shader_load(render_backend_t* backend, const uuid_t uuid) {
	object_t shaderobj = render_backend_shader_acquire(backend, uuid);
	if (shaderobj && render_backend_shader_resolve(backend, shaderobj))
		return shaderobj;

#if RESOURCE_ENABLE_LOCAL_CACHE
	uint64_t platform = render_backend_resource_platform(backend);
	render_shader_t* shader;
	stream_t* stream;
	resource_header_t header;
	bool success = false;
	bool recompile = false;
	bool recompiled = false;

	error_context_declare_local(
	    char uuidbuf[40];
	    const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid);
	);
	error_context_push(STRING_CONST("loading shader"), STRING_ARGS(uuidstr));

	render_backend_enable_thread(backend);

retry:

	stream = resource_stream_open_static(uuid, platform);
	if (stream) {
		header = resource_stream_read_header(stream);
		if (header.version == RENDER_SHADER_RESOURCE_VERSION) {
			if (header.type == HASH_PIXELSHADER)
				shader = render_pixelshader_allocate();
			else if (header.type == HASH_VERTEXSHADER)
				shader = render_vertexshader_allocate();
			if (shader) {
				stream_read(stream, shader, sizeof(render_shader_t));
				shader->backend = nullptr;
			}
		}
		if (!shader && !recompiled) {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected type/version %" PRIx64 " : %u"),
			          (uint64_t)header.type, (uint32_t)header.version);
			recompile = true;
		}
		stream_deallocate(stream);
		stream = nullptr;
	}
	if (shader)
		stream = resource_stream_open_dynamic(uuid, platform);
	if (stream) {
		char* buffer;
		uint32_t version = stream_read_uint32(stream);
		size_t size = (size_t)stream_read_uint64(stream);
		if ((version == RENDER_SHADER_RESOURCE_VERSION) && (size < 128 * 1024))  {
			buffer = memory_allocate(HASH_RENDER, size + 1, 0, MEMORY_TEMPORARY);
			if (stream_read(stream, buffer, size) == size) {
				buffer[size] = 0;
				success = render_shader_upload(backend, shader, buffer, size);
			}
			memory_deallocate(buffer);
		}
		else if (!recompiled) {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected version/size when loading blob: %u (%" PRIu64 ")"),
			          version, size);
			recompile = true;
		}
		stream_deallocate(stream);
		stream = nullptr;
	}

	if (success) {
		shaderobj = render_backend_shader_store(backend, uuid, shader);
	}
	else {
		render_shader_deallocate(shader);
		shader = nullptr;

		if (recompile && !recompiled) {
			recompiled = resource_compile(uuid, render_backend_resource_platform(backend));
			if (recompiled)
				goto retry;
		}
	}

	error_context_pop();

#endif

	return shaderobj;
}

bool
render_shader_reload(render_shader_t* shader, const uuid_t uuid) {
	error_context_declare_local(
	    char uuidbuf[40];
	    const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid);
	);
	error_context_push(STRING_CONST("reloading shader"), STRING_ARGS(uuidstr));

	render_backend_t* backend = shader->backend;
	render_backend_enable_thread(backend);

	bool success = false;
	render_shader_t tmpshader;
	tmpshader.shadertype = 0;

	uint64_t platform = render_backend_resource_platform(backend);

	stream_t* stream = resource_stream_open_static(uuid, platform);
	if (stream) {
		resource_header_t header = resource_stream_read_header(stream);
		if (header.version == RENDER_SHADER_RESOURCE_VERSION) {
			if ((header.type == HASH_PIXELSHADER) && (shader->shadertype == SHADER_PIXEL))
				render_pixelshader_initialize(&tmpshader);
			else if ((header.type == HASH_VERTEXSHADER) && (shader->shadertype == SHADER_VERTEX))
				render_vertexshader_initialize(&tmpshader);
			if (tmpshader.shadertype)
				stream_read(stream, &tmpshader, sizeof(render_shader_t));
		}
		stream_deallocate(stream);
		stream = nullptr;
	}
	if (tmpshader.shadertype)
		stream = resource_stream_open_dynamic(uuid, platform);
	if (stream) {
		char* buffer;
		uint32_t version = stream_read_uint32(stream);
		size_t size = (size_t)stream_read_uint64(stream);
		if ((version == RENDER_SHADER_RESOURCE_VERSION) && (size < 128 * 1024))  {
			buffer = memory_allocate(HASH_RENDER, size + 1, 0, MEMORY_TEMPORARY);
			if (stream_read(stream, buffer, size) == size) {
				buffer[size] = 0;
				success = render_shader_upload(backend, &tmpshader, buffer, size);
			}
			memory_deallocate(buffer);
		}
		else {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected version/size when loading blob: %u (%" PRIu64 ")"),
			          version, size);
		}
		stream_deallocate(stream);
		stream = nullptr;
	}

	if (success) {
		uintptr_t swapdata[4];
		memcpy(swapdata, shader->backend_data, sizeof(swapdata));
		memcpy(shader->backend_data, tmpshader.backend_data, sizeof(shader->backend_data));
		memcpy(tmpshader.backend_data, swapdata, sizeof(swapdata));

		//render_backend_relink_programs(...)
	}

	backend->vtable.deallocate_shader(backend, &tmpshader);

	error_context_pop();

	return success;
}
