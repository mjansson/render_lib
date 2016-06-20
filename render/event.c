/* event.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include <render/event.h>
#include <render/backend.h>
#include <render/shader.h>
#include <render/program.h>

#include <resource/event.h>

void
render_event_handle_resource(render_backend_t* backend, const event_t* event) {
	if (event->id != RESOURCEEVENT_MODIFY)
		return;

	const uuid_t uuid = resource_event_uuid(event);

	render_shader_t* shader = render_backend_shader_lookup(backend, uuid);
	if (shader && render_shader_reload(shader, uuid))
		return;

	render_program_t* program = render_backend_program_lookup(backend, uuid);
	if (program && render_program_reload(program, uuid))
		return;
}
