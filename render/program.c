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

#include <foundation/foundation.h>

#include <render/render.h>

#include <resource/stream.h>
#include <resource/platform.h>
#include <resource/compile.h>

//Size expectations for the program compiler and loader
FOUNDATION_STATIC_ASSERT(sizeof(render_vertex_decl_t) == 72, "invalid vertex decl size");
FOUNDATION_STATIC_ASSERT(sizeof(render_program_t) == 80 + sizeof(render_vertex_decl_t) +
                         (sizeof(hash_t) * VERTEXATTRIBUTE_NUMATTRIBUTES) + 8,
                         "invalid program size");

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
	program->num_parameters = (unsigned int)num_parameters;
}

void
render_program_finalize(render_program_t* program) {
	if (program->backend) {
		if (program->vertexshader && program->vertexshader->id)
			render_backend_shader_release(program->backend, program->vertexshader->id);
		if (program->pixelshader && program->pixelshader->id)
			render_backend_shader_release(program->backend, program->pixelshader->id);
		program->backend->vtable.deallocate_program(program->backend, program);
	}
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

object_t
render_program_load(render_backend_t* backend, const uuid_t uuid) {
	object_t programobj = render_backend_program_acquire(backend, uuid);
	if (programobj && render_backend_program_resolve(backend, programobj))
		return programobj;

#if RESOURCE_ENABLE_LOCAL_CACHE
	const uint32_t expected_version = RENDER_PROGRAM_RESOURCE_VERSION;
	uint64_t platform = render_backend_resource_platform(backend);
	stream_t* stream = nullptr;
	void* block;
	bool success = false;
	uuid_t* shaderuuid;
	size_t remain;
	resource_header_t header;
	object_t vsobj = 0;
	object_t psobj = 0;
	render_shader_t* vshader = nullptr;
	render_shader_t* pshader = nullptr;
	bool recompiled = false;
	render_program_t* program = nullptr;

	error_context_declare_local(
	    char uuidbuf[40];
	    const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid);
	);
	error_context_push(STRING_CONST("loading program"), STRING_ARGS(uuidstr));

	render_backend_enable_thread(backend);

retry:

	stream = resource_stream_open_static(uuid, platform);
	if (!stream)
		goto finalize;

	header = resource_stream_read_header(stream);
	if ((header.type != HASH_PROGRAM) || (header.version != expected_version)) {
		if (!recompiled) {
			log_warnf(HASH_RENDER, WARNING_INVALID_VALUE,
			          STRING_CONST("Got unexpected type/version: %" PRIx64 " : %u"),
			          (uint64_t)header.type, (uint32_t)header.version);
			stream_deallocate(stream);
			stream = nullptr;
			recompiled = resource_compile(uuid, render_backend_resource_platform(backend));
			if (recompiled)
				goto retry;
			log_error(HASH_RENDER, ERROR_INTERNAL_FAILURE, STRING_CONST("Failed recompiling program"));
		}
		goto finalize;
	}

	remain = stream_size(stream) - stream_tell(stream);
	block = memory_allocate(HASH_RENDER, remain, 8, MEMORY_PERSISTENT);

	stream_read(stream, block, remain);
	stream_deallocate(stream);
	stream = nullptr;

	shaderuuid = block;
	vsobj = render_shader_load(backend, *shaderuuid);
	vshader = render_backend_shader_resolve(backend, vsobj);
	if (!vshader || !(vshader->shadertype & SHADER_VERTEX)) {
		log_warn(HASH_RENDER, WARNING_INVALID_VALUE, STRING_CONST("Got invalid vertex shader"));
		goto finalize;
	}

	++shaderuuid;
	psobj = render_shader_load(backend, *shaderuuid);
	pshader = render_backend_shader_resolve(backend, psobj);
	if (!pshader || !(pshader->shadertype & SHADER_PIXEL)) {
		log_warn(HASH_RENDER, WARNING_INVALID_VALUE, STRING_CONST("Got invalid pixel shader"));
		goto finalize;
	}

	program = block;
	program->backend = 0;
	program->vertexshader = vshader;
	program->pixelshader = pshader;
	memset(program->backend_data, 0, sizeof(program->backend_data));

	success = render_program_upload(backend, program);

finalize:
	if (stream)
		stream_deallocate(stream);

	if (success) {
		programobj = render_backend_program_store(backend, uuid, program);
	}
	else {
		render_backend_shader_release(backend, psobj);
		render_backend_shader_release(backend, vsobj);
		render_program_deallocate(program);
		program = nullptr;
	}

	error_context_pop();

#endif

	return programobj;
}

bool
render_program_reload(render_program_t* program, const uuid_t uuid) {
	FOUNDATION_UNUSED(program);
	FOUNDATION_UNUSED(uuid);
	return false;
}
