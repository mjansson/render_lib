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

render_program_t*
render_program_allocate(size_t num_parameters) {
	size_t size = sizeof(render_program_t) + (sizeof(render_parameter_t) * num_parameters);
	render_program_t* program = memory_allocate(HASH_RENDER, size, 0, MEMORY_PERSISTENT);
	render_program_initialize(program, num_parameters);
	return program;
}

void
render_program_initialize(render_program_t* program, size_t num_parameters) {
	memset(program, 0, sizeof(render_program_t));
	program->parameters.num_parameters = num_parameters;
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
render_program_upload(render_program_t* program) {
	return program->backend->vtable.upload_program(program->backend, program);
}
