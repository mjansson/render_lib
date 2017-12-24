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
#include <render/internal.h>

#include <resource/stream.h>
#include <resource/platform.h>
#include <resource/compile.h>

//Size expectations for the program compiler and loader
FOUNDATION_STATIC_ASSERT(sizeof(render_vertex_decl_t) == 72, "invalid vertex decl size");
FOUNDATION_STATIC_ASSERT(sizeof(render_program_t) == 96 + sizeof(render_vertex_decl_t) +
                         (sizeof(hash_t) * VERTEXATTRIBUTE_NUMATTRIBUTES),
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
	program->parameters = program->inline_parameters;
}

void
render_program_finalize(render_program_t* program) {
	render_shader_unload(program->vertexshader);
	render_shader_unload(program->pixelshader);
	if (program->backend) {
		uuidmap_erase(render_backend_program_table(program->backend), program->uuid);
		program->backend->vtable.deallocate_program(program->backend, program);
	}
	if (program->parameters != program->inline_parameters)
		memory_deallocate(program->parameters);
}

void
render_program_deallocate(render_program_t* program) {
	if (program)
		render_program_finalize(program);
	memory_deallocate(program);
}

render_program_t*
render_program_lookup(render_backend_t* backend, const uuid_t uuid) {
	render_program_t* program = uuidmap_lookup(render_backend_program_table(backend), uuid);
	if (program)
		++program->ref;
	return program;
}

static render_program_t*
render_program_load_impl(render_backend_t* backend, const uuid_t uuid) {
	const uint32_t expected_version = RENDER_PROGRAM_RESOURCE_VERSION;
	uint64_t platform = render_backend_resource_platform(backend);
	render_program_t* program = nullptr;
	stream_t* stream = nullptr;
	void* block;
	bool success = false;
	uuid_t* shaderuuid;
	size_t remain;
	resource_header_t header;
	render_shader_t* vshader = nullptr;
	render_shader_t* pshader = nullptr;
	bool recompiled = false;

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
	vshader = render_shader_load(backend, *shaderuuid);
	if (!vshader || !(vshader->shadertype & SHADER_VERTEX)) {
		log_warn(HASH_RENDER, WARNING_INVALID_VALUE, STRING_CONST("Got invalid vertex shader"));
		goto finalize;
	}

	++shaderuuid;
	pshader = render_shader_load(backend, *shaderuuid);
	if (!pshader || !(pshader->shadertype & SHADER_PIXEL)) {
		log_warn(HASH_RENDER, WARNING_INVALID_VALUE, STRING_CONST("Got invalid pixel shader"));
		goto finalize;
	}

	program = block;
	program->backend = 0;
	program->vertexshader = vshader;
	program->pixelshader = pshader;
	program->parameters = program->inline_parameters;
	memset(program->backend_data, 0, sizeof(program->backend_data));

	success = render_backend_program_upload(backend, program);

finalize:
	if (stream)
		stream_deallocate(stream);

	if (!success) {
		render_program_deallocate(program);
		program = nullptr;
	}

	error_context_pop();

	return program;
}

render_program_t*
render_program_load(render_backend_t* backend, const uuid_t uuid) {
	render_program_t* program = render_program_lookup(backend, uuid);
	if (program)
		return program;

	program = render_program_load_impl(backend, uuid);

	if (program) {
		program->ref = 1;
		program->uuid = uuid;
		uuidmap_insert(render_backend_program_table(backend), uuid, program);
	}

	return program;
}

bool
render_program_reload(render_program_t* program, const uuid_t uuid) {
	error_context_declare_local(
	    char uuidbuf[40];
	    const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid);
	);
	error_context_push(STRING_CONST("reloading program"), STRING_ARGS(uuidstr));

	bool success = false;
	render_program_t* tmpprogram = render_program_load_impl(program->backend, uuid);
	if (tmpprogram) {
		render_shader_unload(program->vertexshader);
		render_shader_unload(program->pixelshader);

		program->vertexshader = tmpprogram->vertexshader;
		program->pixelshader = tmpprogram->pixelshader;
		tmpprogram->vertexshader = nullptr;
		tmpprogram->pixelshader = nullptr;

		uintptr_t swapdata[4];
		memcpy(swapdata, program->backend_data, sizeof(swapdata));
		memcpy(program->backend_data, tmpprogram->backend_data, sizeof(program->backend_data));
		memcpy(tmpprogram->backend_data, swapdata, sizeof(swapdata));

		program->size_parameterdata = tmpprogram->size_parameterdata;

		if (program->num_parameters < tmpprogram->num_parameters) {
			if (program->parameters != program->inline_parameters)
				memory_deallocate(program->parameters);
			program->parameters = memory_allocate(HASH_RENDER,
			                                      sizeof(render_parameter_t) * tmpprogram->num_parameters,
			                                      0, MEMORY_PERSISTENT);
		}
		memcpy(program->parameters, tmpprogram->parameters,
		       sizeof(render_parameter_t) * tmpprogram->num_parameters);
		program->num_parameters = tmpprogram->num_parameters;

		memcpy(&program->attributes, &tmpprogram->attributes, sizeof(program->attributes));
		memcpy(program->attribute_name, tmpprogram->attribute_name, sizeof(program->attribute_name));

		render_program_deallocate(tmpprogram);
	}

	error_context_pop();

	return success;
}

void
render_program_unload(render_program_t* program) {
	if (program && program->ref) {
		if (!--program->ref)
			render_program_deallocate(program);
	}
}
