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

void
render_pixelshader_upload(render_backend_t* backend, render_pixelshader_t* shader,
                          const void* buffer, size_t size) {
	if (shader->backend && (shader->backend != backend))
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
	backend->vtable.upload_shader(backend, (render_shader_t*)shader, buffer, size);
	shader->backend = backend;
}

static void*
render_shader_load(render_backend_t* backend, const uuid_t uuid, hash_t type) {
	void* shader = nullptr;

#if RESOURCE_ENABLE_LOCAL_CACHE
	const uint32_t expected_version = 1;
	uint64_t platform = render_backend_resource_platform(backend);
	stream_t* stream;
	bool success = false;
	stream = resource_stream_open_static(uuid, platform);
	if (stream) {
		hash_t type_hash = stream_read_uint64(stream);
		uint32_t version = stream_read_uint32(stream);
		if ((type_hash == type) && (version == expected_version)) {
			if (type == HASH_PIXELSHADER) {
				shader = render_pixelshader_allocate();
				stream_read(stream, shader, sizeof(render_pixelshader_t));
			}
			else {
				shader = render_vertexshader_allocate();
				stream_read(stream, shader, sizeof(render_vertexshader_t));
			}
		}
		else {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected type/version when loading shader: %" PRIx64 " : %u"),
			          type_hash, version);
		}
		stream_deallocate(stream);
		stream = nullptr;
	}
	if (shader)
		stream = resource_stream_open_dynamic(uuid, platform);
	if (stream) {
		void* buffer;
		uint32_t version = stream_read_uint32(stream);
		size_t size = (size_t)stream_read_uint64(stream);
		if (version == expected_version) {
			buffer = memory_allocate(HASH_RENDER, size, 0, MEMORY_TEMPORARY);
			if (stream_read(stream, buffer, size) == size) {
				if (type == HASH_PIXELSHADER)
					render_pixelshader_upload(backend, shader, buffer, size);
				else
					render_vertexshader_upload(backend, shader, buffer, size);
				success = true;
			}
			memory_deallocate(buffer);
		}
		else {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected type/version when loading shader blob: %u"),
			          version);
		}
		stream_deallocate(stream);
		stream = nullptr;
	}

	if (!success) {
		if (type == HASH_PIXELSHADER)
			render_pixelshader_deallocate(shader);
		else
			render_vertexshader_deallocate(shader);
		shader = nullptr;
	}

#endif

	return shader;
}

render_pixelshader_t*
render_pixelshader_load(render_backend_t* backend, const uuid_t uuid) {
	return render_shader_load(backend, uuid, HASH_PIXELSHADER);
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

void
render_vertexshader_upload(render_backend_t* backend, render_vertexshader_t* shader,
                           const void* buffer, size_t size) {
	if (shader->backend && (shader->backend != backend))
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
	backend->vtable.upload_shader(backend, (render_shader_t*)shader, buffer, size);
	shader->backend = backend;
}

render_vertexshader_t*
render_vertexshader_load(render_backend_t* backend, const uuid_t uuid) {
	return render_shader_load(backend, uuid, HASH_VERTEXSHADER);
}
