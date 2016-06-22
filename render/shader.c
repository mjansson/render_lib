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

static void*
render_shader_load(render_backend_t* backend, const uuid_t uuid, hash_t type);

render_pixelshader_t*
render_pixelshader_allocate(void) {
	render_pixelshader_t* shader = memory_allocate(HASH_RENDER, sizeof(render_pixelshader_t), 0,
	                                               MEMORY_PERSISTENT);
	render_pixelshader_initialize(shader);
	return shader;
}

void
render_pixelshader_initialize(render_pixelshader_t* shader) {
	memset(shader, 0, sizeof(render_pixelshader_t));
	shader->shadertype = SHADER_PIXEL;
}

void
render_pixelshader_finalize(render_pixelshader_t* shader) {
	if (shader->backend)
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
}

void
render_pixelshader_deallocate(render_pixelshader_t* shader) {
	if (shader)
		render_pixelshader_finalize(shader);
	memory_deallocate(shader);
}

bool
render_pixelshader_upload(render_backend_t* backend, render_pixelshader_t* shader,
                          const void* buffer, size_t size) {
	if (shader->backend && (shader->backend != backend))
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
	if (backend->vtable.upload_shader(backend, (render_shader_t*)shader, buffer, size)) {
		shader->backend = backend;
		return true;
	}
	return false;
}

render_pixelshader_t*
render_pixelshader_load(render_backend_t* backend, const uuid_t uuid) {
	return render_shader_load(backend, uuid, HASH_PIXELSHADER);
}

bool
render_pixelshader_reload(render_pixelshader_t* shader, const uuid_t uuid) {
	FOUNDATION_UNUSED(shader);
	FOUNDATION_UNUSED(uuid);
	return false;
}

render_vertexshader_t*
render_vertexshader_allocate(void) {
	render_vertexshader_t* shader = memory_allocate(HASH_RENDER, sizeof(render_vertexshader_t), 16,
	                                                MEMORY_PERSISTENT);
	render_vertexshader_initialize(shader);
	return shader;
}

void
render_vertexshader_initialize(render_vertexshader_t* shader) {
	memset(shader, 0, sizeof(render_vertexshader_t));
	shader->shadertype = SHADER_VERTEX;
}

void
render_vertexshader_finalize(render_vertexshader_t* shader) {
	if (shader->backend)
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
}

void
render_vertexshader_deallocate(render_vertexshader_t* shader) {
	if (shader)
		render_vertexshader_finalize(shader);
	memory_deallocate(shader);
}

bool
render_vertexshader_upload(render_backend_t* backend, render_vertexshader_t* shader,
                           const void* buffer, size_t size) {
	if (shader->backend && (shader->backend != backend))
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
	if (backend->vtable.upload_shader(backend, (render_shader_t*)shader, buffer, size)) {
		shader->backend = backend;
		return true;
	}
	return false;
}

render_vertexshader_t*
render_vertexshader_load(render_backend_t* backend, const uuid_t uuid) {
	return render_shader_load(backend, uuid, HASH_VERTEXSHADER);
}

bool
render_vertexshader_reload(render_vertexshader_t* shader, const uuid_t uuid) {
	FOUNDATION_UNUSED(shader);
	FOUNDATION_UNUSED(uuid);
	return false;
}

static void*
render_shader_load(render_backend_t* backend, const uuid_t uuid, hash_t type) {
	void* shader = render_backend_shader_lookup(backend, uuid);
	if (shader) {
		atomic_incr32(&((render_shader_t*)shader)->ref);
		return shader;
	}

#if RESOURCE_ENABLE_LOCAL_CACHE
	const uint32_t expected_version = RENDER_SHADER_RESOURCE_VERSION;
	uint64_t platform = render_backend_resource_platform(backend);
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

retry:

	stream = resource_stream_open_static(uuid, platform);
	render_backend_enable_thread(backend);
	if (stream) {
		header = resource_stream_read_header(stream);
		if ((header.type == type) && (header.version == expected_version)) {
			if (type == HASH_PIXELSHADER) {
				shader = render_pixelshader_allocate();
				stream_read(stream, shader, sizeof(render_pixelshader_t));
			}
			else {
				shader = render_vertexshader_allocate();
				stream_read(stream, shader, sizeof(render_vertexshader_t));
			}
			((render_shader_t*)shader)->backend = nullptr;
		}
		else if (!recompiled) {
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
		if ((version == expected_version) && (size < 128 * 1024))  {
			buffer = memory_allocate(HASH_RENDER, size + 1, 0, MEMORY_TEMPORARY);
			if (stream_read(stream, buffer, size) == size) {
				buffer[size] = 0;
				if (type == HASH_PIXELSHADER)
					success = render_pixelshader_upload(backend, shader, buffer, size);
				else
					success = render_vertexshader_upload(backend, shader, buffer, size);
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
		atomic_store32(&((render_shader_t*)shader)->ref, 1);
		render_backend_shader_store(backend, uuid, shader);
	}
	else {
		if (type == HASH_PIXELSHADER)
			render_pixelshader_deallocate(shader);
		else
			render_vertexshader_deallocate(shader);
		shader = nullptr;

		if (recompile && !recompiled) {
			recompiled = resource_compile(uuid, render_backend_resource_platform(backend));
			if (recompiled)
				goto retry;
		}
	}

	error_context_pop();

#endif

	return shader;
}

bool
render_shader_reload(render_shader_t* shader, const uuid_t uuid) {
	switch (shader->shadertype) {
		case SHADER_PIXEL:  return render_pixelshader_reload((render_pixelshader_t*)shader, uuid);
		case SHADER_VERTEX: return render_vertexshader_reload((render_vertexshader_t*)shader, uuid);
		default: break;
	}
	return false;
}
