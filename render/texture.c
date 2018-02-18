/* texture.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include <resource/stream.h>
#include <resource/platform.h>
#include <resource/compile.h>

FOUNDATION_STATIC_ASSERT(sizeof(render_texture_t) == 96, "invalid texture size");

render_texture_t*
render_texture_allocate(void) {
	render_texture_t* texture = memory_allocate(HASH_RENDER, sizeof(render_texture_t), 16,
	                                            MEMORY_PERSISTENT);
	render_texture_initialize(texture);
	return texture;

}

void
render_texture_initialize(render_texture_t* texture) {
	memset(texture, 0, sizeof(render_texture_t));
}

void
render_texture_finalize(render_texture_t* texture) {
	if (texture->backend) {
		uuidmap_erase(render_backend_texture_table(texture->backend), texture->uuid);
		texture->backend->vtable.deallocate_texture(texture->backend, texture);
	}
}

void
render_texture_deallocate(render_texture_t* texture) {
	if (texture)
		render_texture_finalize(texture);
	memory_deallocate(texture);
}

render_texture_t*
render_texture_load(render_backend_t* backend, const uuid_t uuid) {
	render_texture_t* texture = render_texture_lookup(backend, uuid);
	if (texture)
		return texture;

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
	error_context_push(STRING_CONST("loading texture"), STRING_ARGS(uuidstr));

	render_backend_enable_thread(backend);

retry:

	stream = resource_stream_open_static(uuid, platform);
	if (stream) {
		header = resource_stream_read_header(stream);		
		if (header.version == RENDER_TEXTURE_RESOURCE_VERSION) {
			if (header.type == HASH_TEXTURE)
				texture = render_texture_allocate();
			if (texture) {
				stream_read(stream, texture, sizeof(render_texture_t));
				texture->backend = nullptr;
			}
		}
		if (!texture && !recompiled) {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected type/version %" PRIx64 " : %u"),
			          (uint64_t)header.type, (uint32_t)header.version);
			recompile = true;
		}
		stream_deallocate(stream);
		stream = nullptr;
	}
	if (texture)
		stream = resource_stream_open_dynamic(uuid, platform);
	if (stream) {
		char* buffer;
		uint32_t version = stream_read_uint32(stream);
		size_t size = (size_t)stream_read_uint64(stream);
		if ((version == RENDER_TEXTURE_RESOURCE_VERSION) && (size < 128 * 1024))  {
			buffer = memory_allocate(HASH_RENDER, size + 1, 0, MEMORY_TEMPORARY);
			if (stream_read(stream, buffer, size) == size) {
				buffer[size] = 0;
				success = render_backend_texture_upload(backend, texture, buffer, size);
			}
			memory_deallocate(buffer);
		}
		else if (!recompiled) {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected version/size when loading blob: %u (%" PRIsize ")"),
			          version, size);
			recompile = true;
		}
		stream_deallocate(stream);
		stream = nullptr;
	}

	if (!success) {
		render_texture_deallocate(texture);
		texture = nullptr;

		if (recompile && !recompiled) {
			recompiled = resource_compile(uuid, render_backend_resource_platform(backend));
			if (recompiled)
				goto retry;
		}
	}

	if (texture) {
		atomic_store32(&texture->ref, 1, memory_order_release);
		texture->uuid = uuid;
		uuidmap_insert(render_backend_texture_table(backend), uuid, texture);
	}

	error_context_pop();

	return texture;
}

render_texture_t*
render_texture_lookup(render_backend_t* backend, const uuid_t uuid) {
	render_texture_t* texture = uuidmap_lookup(render_backend_texture_table(backend), uuid);
	if (texture)
		atomic_incr32(&texture->ref, memory_order_release);
	return texture;

}

bool
render_texture_reload(render_texture_t* texture, const uuid_t uuid) {
	error_context_declare_local(
	    char uuidbuf[40];
	    const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid);
	);
	error_context_push(STRING_CONST("reloading texture"), STRING_ARGS(uuidstr));

	render_backend_t* backend = texture->backend;
	render_backend_enable_thread(backend);

	bool success = false;
	render_texture_t tmptexture;

	uint64_t platform = render_backend_resource_platform(backend);

	stream_t* stream = resource_stream_open_static(uuid, platform);
	if (stream) {
		resource_header_t header = resource_stream_read_header(stream);
		if (header.version == RENDER_TEXTURE_RESOURCE_VERSION) {
			if (header.type == HASH_TEXTURE) {
				success = true;
				stream_read(stream, &tmptexture, sizeof(render_texture_t));
			}
		}
		stream_deallocate(stream);
		stream = nullptr;
	}
	if (success) {
		success = false;
		stream = resource_stream_open_dynamic(uuid, platform);
	}
	if (stream) {
		char* buffer;
		uint32_t version = stream_read_uint32(stream);
		size_t size = (size_t)stream_read_uint64(stream);
		if ((version == RENDER_TEXTURE_RESOURCE_VERSION) && (size < 128 * 1024))  {
			buffer = memory_allocate(HASH_RENDER, size + 1, 0, MEMORY_TEMPORARY);
			if (stream_read(stream, buffer, size) == size) {
				buffer[size] = 0;
				success = render_backend_texture_upload(backend, &tmptexture, buffer, size);
			}
			memory_deallocate(buffer);
		}
		else {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected version/size when loading blob: %u (%" PRIsize ")"),
			          version, size);
		}
		stream_deallocate(stream);
		stream = nullptr;
	}

	if (success) {
		uintptr_t swapdata[4];
		memcpy(swapdata, texture->backend_data, sizeof(swapdata));
		memcpy(texture->backend_data, tmptexture.backend_data, sizeof(texture->backend_data));
		memcpy(tmptexture.backend_data, swapdata, sizeof(swapdata));
		texture->pixelformat = tmptexture.pixelformat;
		texture->colorspace = tmptexture.colorspace;
		texture->width = tmptexture.width;
		texture->height = tmptexture.height;
		texture->depth = tmptexture.depth;
		texture->levels = tmptexture.levels;
	}

	backend->vtable.deallocate_texture(backend, &tmptexture);

	error_context_pop();

	return success;
}

void
render_texture_unload(render_texture_t* texture) {
	if (texture && atomic_load32(&texture->ref, memory_order_acquire)) {
		if (!atomic_decr32(&texture->ref, memory_order_release))
			render_texture_deallocate(texture);
	}	
}
