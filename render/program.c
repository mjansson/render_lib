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
render_program_allocate(void) {
	render_program_t* program = memory_allocate(HASH_RENDER, sizeof(render_program_t), 0,
	                                            MEMORY_PERSISTENT);
	render_program_initialize(program);
	return program;
}

void
render_program_initialize(render_program_t* program) {
	memset(program, 0, sizeof(render_program_t));
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
