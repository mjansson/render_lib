/* event.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/render_lib
 *
 * The dependent library source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <render/event.h>
#include <render/backend.h>
#include <render/shader.h>
#include <render/program.h>
#include <render/hashstrings.h>

#include <foundation/array.h>
#include <foundation/log.h>

#include <resource/event.h>

void
render_event_handle_resource(const event_t* event) {
	if ((event->id != RESOURCEEVENT_MODIFY) && (event->id != RESOURCEEVENT_DEPENDS))
		return;

	const uuid_t uuid = resource_event_uuid(event);

	render_backend_t** backends = render_backends();
	for (size_t ib = 0, bsize = array_size(backends); ib < bsize; ++ib) {
		render_backend_t* backend = backends[ib];

		render_shader_t* shader = render_shader_lookup(backend, uuid);
		if (shader) {
			string_const_t uuidstr = string_from_uuid_static(uuid);
			log_debugf(HASH_RENDER, STRING_CONST("Resource event trigger shader reload: %.*s"), STRING_FORMAT(uuidstr));

			render_shader_reload(shader, uuid);
			render_shader_unload(shader);
			continue;
		}

		render_program_t* program = render_program_lookup(backend, uuid);
		if (program) {
			string_const_t uuidstr = string_from_uuid_static(uuid);
			log_debugf(HASH_RENDER, STRING_CONST("Resource event trigger program reload: %.*s"),
			           STRING_FORMAT(uuidstr));

			render_program_reload(program, uuid);
			render_program_unload(program);
			continue;
		}
	}
}
