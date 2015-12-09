/* glsl.c  -  Render library importer  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include "errorcodes.h"
#include "glsl.h"

#define TOKEN_DELIM " \t\n\r;[](){}.,"

int
renderimport_import_glsl_vertexshader(stream_t* stream, const uuid_t uuid) {
	resource_source_t source;
	void* blob;
	size_t size;
	size_t read;
	size_t begin;
	size_t next;
	hash_t checksum;
	tick_t timestamp;
	uint64_t platform;
	size_t maxtokens;
	size_t parameter;
	string_const_t* token = 0;
	string_const_t valstr;
	char buffer[128];
	int ret = RENDERIMPORT_RESULT_OK;

	resource_source_initialize(&source);
	resource_source_read(&source, uuid);

	read = 0;
	size = stream_size(stream);
	blob = memory_allocate(HASH_RESOURCE, size, 0, MEMORY_PERSISTENT);

	size = stream_read(stream, blob, size);

	platform = 0;
	timestamp = time_system();
	checksum = hash(blob, size);
	if (resource_source_write_blob(uuid, timestamp, HASH_SOURCE,
	                               platform, checksum, blob, size)) {
		resource_source_set_blob(&source, timestamp, HASH_SOURCE,
		                         platform, checksum, size);
	}
	else {
		ret = RENDERIMPORT_RESULT_UNABLE_TO_WRITE_BLOB;
		goto finalize;
	}

	//Parse source and set parameters
	maxtokens = 256;
	token = memory_allocate(HASH_RESOURCE, sizeof(string_const_t) * maxtokens, 0, MEMORY_PERSISTENT);
	begin = 0;
	parameter = 0;
	do {
		string_const_t token[64], line;
		size_t itok, tokens;

		next = string_find_first_of(blob, size, STRING_CONST("\n\r"), begin);
		line = string_substr(blob, size, begin, next - begin);
		tokens = string_explode(STRING_ARGS(line), STRING_CONST(TOKEN_DELIM),
		                        token, maxtokens, false);

		for (itok = 0; itok < tokens; ++itok) {
			if ((string_equal(STRING_ARGS(token[itok]), STRING_CONST("attribute")) ||
			        string_equal(STRING_ARGS(token[itok]), STRING_CONST("uniform"))) &&
			        (itok + 2 < tokens)) {
				string_const_t type = token[itok + 1];
				string_const_t name = token[itok + 2];
				string_t param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_type_%" PRIsize),
				                               parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)),
				                    platform, STRING_ARGS(type));
				param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_name_%" PRIsize),
				                      name);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)),
				                    platform, STRING_ARGS(type));
				++parameter;
			}
		}

		begin = string_find_first_not_of(blob, size, STRING_CONST(STRING_WHITESPACE), next);
	}
	while (next != STRING_NPOS);

	valstr = string_from_uint_static(parameter, false, 0, 0);
	resource_source_set(&source, timestamp, HASH_PARAMETER_COUNT,
	                    platform, STRING_ARGS(valstr));

	if (!resource_source_write(&source, uuid, false)) {
		ret = RENDERIMPORT_RESULT_UNABLE_TO_WRITE_SOURCE;
		goto finalize;
	}

finalize:
	memory_deallocate(token);
	resource_source_finalize(&source);

	return ret;
}

int
renderimport_import_glsl_pixelshader(stream_t* stream, const uuid_t uuid) {
	return renderimport_import_glsl_vertexshader(stream, uuid);
}
