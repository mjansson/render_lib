/* program.c  -  Render library importer  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include "program.h"
#include "shader.h"

int
renderimport_import_program(stream_t* stream, const uuid_t uuid) {
	char buffer[1024];
	char pathbuf[BUILD_MAX_PATHLEN];
	resource_source_t source;
	resource_platform_t platformdecl = {-1, -1, -1, -1, -1};
	uint64_t platform;
	tick_t timestamp;
	int ret = 0;

	resource_source_initialize(&source);
	resource_source_read(&source, uuid);

	platform = resource_platform(platformdecl);
	timestamp = time_system();

	while (!stream_eos(stream)) {
		string_const_t type, ref;
		string_const_t fullpath;
		string_const_t uuidstr;
		uuid_t shaderuuid;
		hash_t typehash;
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		string_split(STRING_ARGS(line), STRING_CONST(" \t"),
		             &type, &ref, false);
		
		type = string_strip(STRING_ARGS(type), STRING_CONST(STRING_WHITESPACE)); 
		ref = string_strip(STRING_ARGS(ref), STRING_CONST(STRING_WHITESPACE));
		if (!type.length || !ref.length)
			continue;

		typehash = hash(STRING_ARGS(type));
		if ((typehash != HASH_VERTEXSHADER) && (typehash != HASH_PIXELSHADER)) {
			log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Ignore invalid line: %.*s"),
			          STRING_FORMAT(line));
			continue;
		}

		shaderuuid = string_to_uuid(STRING_ARGS(ref));
		if (uuid_is_null(shaderuuid)) {
			if (path_is_absolute(STRING_ARGS(ref))) {
				fullpath = ref;
			}
			else {
				string_t full;
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				full = path_concat(pathbuf, sizeof(pathbuf),
				                   STRING_ARGS(path), STRING_ARGS(ref));
				fullpath = string_const(STRING_ARGS(full));
			}

			shaderuuid = resource_import_map_lookup(STRING_ARGS(fullpath));
			if (uuid_is_null(shaderuuid)) {
				if (!resource_import(STRING_ARGS(fullpath), uuid_null())) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Unable to import linked shader: %.*s"),
					          STRING_FORMAT(fullpath));
					ret = RENDERIMPORT_RESULT_INVALID_INPUT;
					goto finalize;
				}
				shaderuuid = resource_import_map_lookup(STRING_ARGS(fullpath));
				if (!uuid_is_null(shaderuuid)) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Import linked shader gave no UUID: %.*s"),
					          STRING_FORMAT(fullpath));
					ret = RENDERIMPORT_RESULT_INVALID_INPUT;
					goto finalize;
				}
			}
		}

		if (!uuid_is_null(shaderuuid)) {
			uuidstr = string_from_uuid_static(shaderuuid);
			resource_source_set(&source, timestamp, typehash,
			                    platform, STRING_ARGS(uuidstr));
		}
	}

	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE,
	                    0, STRING_CONST("program"));

	if (!resource_source_write(&source, uuid, false)) {
		ret = RENDERIMPORT_RESULT_UNABLE_TO_WRITE_SOURCE;
		goto finalize;
	}

finalize:
	resource_source_finalize(&source);

	return ret;
}
