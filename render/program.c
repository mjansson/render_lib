/* program.c  -  Render library  -  Public Domain  -  2015 Mattias Jansson / Rampant Pixels
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

#include <foundation/foundation.h>

#include <render/render.h>

#include <resource/stream.h>
#include <resource/platform.h>

//Size expectations for the program compiler and loader
FOUNDATION_STATIC_ASSERT(sizeof(render_vertex_decl_t) == 72, "invalid vertex decl size");
FOUNDATION_STATIC_ASSERT(sizeof(render_parameter_decl_t) == 8, "invalid parameter decl size");
FOUNDATION_STATIC_ASSERT(sizeof(render_program_t) == 64 + sizeof(render_vertex_decl_t) +
                         (sizeof(hash_t) * VERTEXATTRIBUTE_NUMATTRIBUTES) +
                         sizeof(render_parameter_decl_t), "invalid program size");

render_program_t*
render_program_allocate(size_t num_parameters) {
	size_t size = sizeof(render_program_t) + (sizeof(render_parameter_t) * num_parameters);
	render_program_t* program = memory_allocate(HASH_RENDER, size, 8, MEMORY_PERSISTENT);
	render_program_initialize(program, num_parameters);
	return program;
}

void
render_program_initialize(render_program_t* program, size_t num_parameters) {
	memset(program, 0, sizeof(render_program_t));
	program->parameters.num_parameters = (unsigned int)num_parameters;
}

void
render_program_finalize(render_program_t* program) {
	if (program->backend)
		program->backend->vtable.deallocate_program(program->backend, program);
}

void
render_program_deallocate(render_program_t* program) {
	if (program)
		render_program_finalize(program);
	memory_deallocate(program);
}

bool
render_program_upload(render_backend_t* backend, render_program_t* program) {
	if (program->backend && (program->backend != backend))
		program->backend->vtable.deallocate_program(program->backend, program);
	program->backend = backend;
	return backend->vtable.upload_program(backend, program);
}

render_program_t*
render_program_load(render_backend_t* backend, const uuid_t uuid) {
	render_program_t* program = nullptr;

#if RESOURCE_ENABLE_LOCAL_CACHE
	const uint32_t expected_version = 1;
	uint64_t platform = render_backend_resource_platform(backend);
	stream_t* stream;
	bool success = false;
	hash_t type_hash;
	uint32_t version;
	uuid_t shaderuuid;
	size_t remain, uuid_size;
	render_vertexshader_t* vertexshader = 0;
	render_pixelshader_t* pixelshader = 0;

	stream = resource_stream_open_static(uuid, platform);
	if (!stream)
		goto finalize;

	type_hash = stream_read_uint64(stream);
	version = stream_read_uint32(stream);
	if ((type_hash != HASH_PROGRAM) || (version != expected_version)) {
		log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
		          STRING_CONST("Got unexpected type/version when loading program: %" PRIx64 " : %u"),
		          type_hash, version);
		goto finalize;
	}

	//TODO: Local dependency tracking, check if shaders need recompilation

	shaderuuid = stream_read_uint128(stream);
	vertexshader = render_vertexshader_load(backend, shaderuuid);
	if (!vertexshader) {
		string_const_t uuidstr = string_from_uuid_static(shaderuuid);
		log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
		          STRING_CONST("Got invalid vertex shader when loading program: %.*s"),
		          STRING_FORMAT(uuidstr));
		goto finalize;
	}

	shaderuuid = stream_read_uint128(stream);
	pixelshader = render_pixelshader_load(backend, shaderuuid);
	if (!pixelshader) {
		string_const_t uuidstr = string_from_uuid_static(shaderuuid);
		log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
		          STRING_CONST("Got invalid pixel shader when loading program: %.*s"),
		          STRING_FORMAT(uuidstr));
		goto finalize;
	}

	uuid_size = sizeof(uuid_t) * 2;
	remain = stream_size(stream) - stream_tell(stream);
	program = memory_allocate(HASH_RENDER, uuid_size + remain, 8, MEMORY_PERSISTENT);

	stream_read(stream, pointer_offset(program, uuid_size), remain);

	program->backend = 0;
	program->vertexshader = vertexshader;
	program->pixelshader = pixelshader;
	memset(program->backend_data, 0, sizeof(program->backend_data));

	success = render_program_upload(backend, program);

finalize:
	stream_deallocate(stream);

	if (!success) {
		render_pixelshader_deallocate(pixelshader);
		render_vertexshader_deallocate(vertexshader);
		render_program_deallocate(program);
		program = nullptr;
	}
#endif

	return program;
}
