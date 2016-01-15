/* shader.h  -  Render library importer  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>
#include <render/render.h>
#include <resource/resource.h>

#include "shader.h"

uuid_t
renderimport_shader_check_referenced_uuid(stream_t* stream) {
	char buffer[1024];
	uuid_t uuid = uuid_null();

	while (!stream_eos(stream)) {
		string_const_t ref;
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		string_split(STRING_ARGS(line), STRING_CONST(" \t"),
		             nullptr, &ref, false);
		ref = string_strip(STRING_ARGS(ref), STRING_CONST(STRING_WHITESPACE));
		if (ref.length) {
			if (path_is_absolute(STRING_ARGS(ref))) {
				uuid = resource_import_map_lookup(STRING_ARGS(ref));
			}
			else {
				char pathbuf[BUILD_MAX_PATHLEN];
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				string_t fullpath = path_concat(pathbuf, sizeof(pathbuf),
				                                STRING_ARGS(path), STRING_ARGS(ref));
				uuid = resource_import_map_lookup(STRING_ARGS(fullpath));
			}
			if (!uuid_is_null(uuid))
				break;
		}
	}
	stream_seek(stream, 0, STREAM_SEEK_BEGIN);

	return uuid;
}

renderimport_type_t
renderimport_shader_guess_type(stream_t* stream) {
	renderimport_type_t type = IMPORTTYPE_UNKNOWN;
	char buffer[1024];
	while (!stream_eos(stream)) {
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		if (string_find_string(STRING_ARGS(line), STRING_CONST("gl_FragColor"), 0) != STRING_NPOS) {
			type = IMPORTTYPE_GLSL_PIXELSHADER;
			break;
		}
		else if (string_find_string(STRING_ARGS(line), STRING_CONST("gl_Position"), 0) != STRING_NPOS) {
			type = IMPORTTYPE_GLSL_VERTEXSHADER;
			break;
		}
	}

	stream_seek(stream, 0, STREAM_SEEK_BEGIN);

	return type;
}

int
renderimport_import_shader(stream_t* stream, const uuid_t uuid) {
	char buffer[1024];
	char pathbuf[BUILD_MAX_PATHLEN];

	while (!stream_eos(stream)) {
		string_const_t ref;
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		string_split(STRING_ARGS(line), STRING_CONST(" \t"),
		             nullptr, &ref, false);
		ref = string_strip(STRING_ARGS(ref), STRING_CONST(STRING_WHITESPACE));
		if (ref.length) {
			string_const_t fullpath;
			uuid_t shaderuuid;
			if (path_is_absolute(STRING_ARGS(ref))) {
				fullpath = ref;
			}
			else {
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				path = path_concat(pathbuf, sizeof(pathbuf),
				                   STRING_ARGS(path), STRING_ARGS(ref));
				fullpath = string_const(STRING_ARGS(path));
			}
			shaderuuid = resource_import_map_lookup(STRING_ARGS(ref));
			if (!uuid_is_null(uuid) && !uuid_equal(uuid, shaderuuid)) {
				log_warn(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Shader UUID mismatch"));
				return RENDERIMPORT_RESULT_UUID_MISMATCH;
			}

			if (renderimport_import())
		}
	}

	return 0;
}
