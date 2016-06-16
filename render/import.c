/* import.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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
#include <resource/resource.h>

#if RESOURCE_ENABLE_LOCAL_SOURCE

typedef enum {
	IMPORTTYPE_UNKNOWN,
	IMPORTTYPE_SHADER,
	IMPORTTYPE_GLSL_VERTEXSHADER,
	IMPORTTYPE_GLSL_PIXELSHADER,
	IMPORTTYPE_PROGRAM
} renderimport_type_t;

#define GLSL_TOKEN_DELIM " \t\n\r;(){}.,"

static renderimport_type_t
render_import_shader_guess_type(stream_t* stream) {
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

static uuid_t
render_import_shader_check_referenced_uuid(stream_t* stream) {
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
				uuid = resource_import_map_lookup(STRING_ARGS(ref)).uuid;
			}
			else {
				char pathbuf[BUILD_MAX_PATHLEN];
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				string_t fullpath = path_concat(pathbuf, sizeof(pathbuf),
				                                STRING_ARGS(path), STRING_ARGS(ref));
				uuid = resource_import_map_lookup(STRING_ARGS(fullpath)).uuid;
			}

			if (!uuid_is_null(uuid))
				break;
		}
	}
	stream_seek(stream, 0, STREAM_SEEK_BEGIN);

	return uuid;
}

static int
glsl_type_to_parameter_type(const string_const_t type) {
	if ((type.length >= 4) && string_equal(type.str, 4, STRING_CONST("vec4")))
		return RENDERPARAMETER_FLOAT4;
	else if ((type.length >= 5) && (string_equal(type.str, 5, STRING_CONST("ivec4")) ||
	                                string_equal(type.str, 5, STRING_CONST("uvec4"))))
		return RENDERPARAMETER_INT4;
	else if ((type.length >= 4) && string_equal(type.str, 4, STRING_CONST("mat4")))
		return RENDERPARAMETER_MATRIX;
	else if ((type.length >= 7) && (string_equal(type.str, 6, STRING_CONST("sampler")) ||
	                                string_equal(type.str + 1, 6, STRING_CONST("sampler"))))
		return RENDERPARAMETER_TEXTURE;
	return -1;
}

static int
glsl_dim_from_token(const string_const_t token) {
	string_const_t dim;
	size_t ofs = string_find(STRING_ARGS(token), '[', 0);
	if (ofs == STRING_NPOS)
		return 1;
	++ofs;
	dim = string_substr(STRING_ARGS(token), ofs, string_find(STRING_ARGS(token), ']', ofs) - ofs);
	return string_to_int(STRING_ARGS(dim));
}

static string_const_t
glsl_name_from_token(const string_const_t token) {
	size_t ofs = string_find_first_of(STRING_ARGS(token), STRING_CONST(GLSL_TOKEN_DELIM "[]"), 0);
	return string_substr(STRING_ARGS(token), 0, ofs);
}

static int
render_import_glsl_shader(stream_t* stream, const uuid_t uuid, const char* type, size_t type_length) {
	resource_source_t source;
	void* blob = 0;
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
	int ret = 0;
	resource_platform_t platformdecl = {-1, RENDERAPIGROUP_OPENGL, -1, -1, -1};

	resource_source_initialize(&source);
	resource_source_read(&source, uuid);

	read = 0;
	size = stream_size(stream);
	blob = memory_allocate(HASH_RESOURCE, size, 0, MEMORY_PERSISTENT);

	size = stream_read(stream, blob, size);

	platform = resource_platform(platformdecl);
	timestamp = time_system();
	checksum = hash(blob, size);
	if (resource_source_write_blob(uuid, timestamp, HASH_SOURCE,
	                               platform, checksum, blob, size)) {
		resource_source_set_blob(&source, timestamp, HASH_SOURCE,
		                         platform, checksum, size);
	}
	else {
		ret = -1;
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
		tokens = string_explode(STRING_ARGS(line), STRING_CONST(GLSL_TOKEN_DELIM),
		                        token, maxtokens, false);

		for (itok = 0; itok < tokens; ++itok) {
			if ((string_equal(STRING_ARGS(token[itok]), STRING_CONST("attribute")) ||
			        string_equal(STRING_ARGS(token[itok]), STRING_CONST("uniform"))) &&
			        (itok + 2 < tokens)) {
				char typebuf[16], dimbuf[16];
				string_const_t type = token[itok + 1];
				string_const_t name = token[itok + 2];
				string_const_t dim;

				int parameter_type = glsl_type_to_parameter_type(type);
				if (parameter_type < 0)
					continue;

				int parameter_dim = glsl_dim_from_token(type);
				if (parameter_dim == 1)
					parameter_dim = glsl_dim_from_token(name);

				name = glsl_name_from_token(name);
				type = string_to_const(string_from_uint(typebuf, sizeof(typebuf), parameter_type, false, 0, 0));
				dim = string_to_const(string_from_uint(dimbuf, sizeof(dimbuf), parameter_dim, false, 0, 0));

				log_debugf(HASH_RESOURCE, STRING_CONST("parameter: %.*s type %.*s dim %.*s"),
				          STRING_FORMAT(name), STRING_FORMAT(type), STRING_FORMAT(dim));

				string_t param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_type_%" PRIsize),
				                               parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)),
				                    platform, STRING_ARGS(type));

				param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_name_%" PRIsize),
				                      parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)),
				                    platform, STRING_ARGS(name));

				param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_dim_%" PRIsize),
				                      parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)),
				                    platform, STRING_ARGS(dim));
				++parameter;
			}
		}

		begin = string_find_first_not_of(blob, size, STRING_CONST(STRING_WHITESPACE), next);
	}
	while (next != STRING_NPOS);

	valstr = string_from_uint_static(parameter, false, 0, 0);
	resource_source_set(&source, timestamp, HASH_PARAMETER_COUNT,
	                    platform, STRING_ARGS(valstr));

	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE,
	                    0, type, type_length);

	if (!resource_source_write(&source, uuid, false)) {
		ret = -1;
		goto finalize;
	}

finalize:
	memory_deallocate(blob);
	memory_deallocate(token);
	resource_source_finalize(&source);

	return ret;
}

static int
render_import_glsl_vertexshader(stream_t* stream, const uuid_t uuid) {
	return render_import_glsl_shader(stream, uuid, STRING_CONST("vertexshader"));
}

static int
render_import_glsl_pixelshader(stream_t* stream, const uuid_t uuid) {
	return render_import_glsl_shader(stream, uuid, STRING_CONST("pixelshader"));
}

static int
render_import_shader(stream_t* stream, const uuid_t uuid) {
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
				string_t full;
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				full = path_concat(pathbuf, sizeof(pathbuf),
				                   STRING_ARGS(path), STRING_ARGS(ref));
				fullpath = string_const(STRING_ARGS(full));
			}

			shaderuuid = resource_import_map_lookup(STRING_ARGS(fullpath)).uuid;
			if (!uuid_is_null(uuid) && !uuid_equal(uuid, shaderuuid)) {
				log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Shader UUID mismatch: %.*s"),
				          STRING_FORMAT(fullpath));
				return -1;
			}

			if (!resource_import(STRING_ARGS(fullpath), uuid)) {
				log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Unable to import linked shader: %.*s"),
				          STRING_FORMAT(fullpath));
				return -1;
			}
		}
	}

	return 0;
}

static int
render_import_program(stream_t* stream, const uuid_t uuid) {
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

			resource_signature_t sig = resource_import_map_lookup(STRING_ARGS(fullpath));
			if (uuid_is_null(sig.uuid)) {
				if (!resource_import(STRING_ARGS(fullpath), uuid_null())) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Unable to import linked shader: %.*s"),
					          STRING_FORMAT(fullpath));
					ret = -1;
					goto finalize;
				}
				sig = resource_import_map_lookup(STRING_ARGS(fullpath));
				if (uuid_is_null(sig.uuid)) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Import linked shader gave no UUID: %.*s"),
					          STRING_FORMAT(fullpath));
					ret = -1;
					goto finalize;
				}
			}
			shaderuuid = sig.uuid;
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
		ret = -1;
		goto finalize;
	}

finalize:
	resource_source_finalize(&source);

	return ret;
}

int
render_import(stream_t* stream, const uuid_t uuid_given) {
	renderimport_type_t type = IMPORTTYPE_UNKNOWN;
	renderimport_type_t guess = IMPORTTYPE_UNKNOWN;
	uuid_t uuid = uuid_given;
	string_const_t path;
	string_const_t extension;
	int ret;
	bool store_import = false;

	path = stream_path(stream);
	extension = path_file_extension(STRING_ARGS(path));
	if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("shader")))
		guess = IMPORTTYPE_SHADER;
	else if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("program")))
		guess = IMPORTTYPE_PROGRAM;

	type = render_import_shader_guess_type(stream);

	if ((type == IMPORTTYPE_UNKNOWN) && (guess != IMPORTTYPE_UNKNOWN))
		type = guess;

	if (type == IMPORTTYPE_UNKNOWN)
		return -1;

	if (uuid_is_null(uuid))
		uuid = resource_import_map_lookup(STRING_ARGS(path)).uuid;

	if (uuid_is_null(uuid) && (type == IMPORTTYPE_SHADER)) {
		uuid = render_import_shader_check_referenced_uuid(stream);
		store_import = true;
	}

	if (uuid_is_null(uuid)) {
		uuid = uuid_generate_random();
		store_import = true;
	}

	if (store_import) {
		uuid_t founduuid = resource_import_map_store(STRING_ARGS(path), uuid, uint256_null());
		if (uuid_is_null(founduuid)) {
			log_warn(HASH_RESOURCE, WARNING_SUSPICIOUS,
			         STRING_CONST("Unable to open import map file to store new resource"));
			return -1;
		}
		uuid = founduuid;
	}

	switch (type) {
	case IMPORTTYPE_PROGRAM:
		ret = render_import_program(stream, uuid);
		break;
	case IMPORTTYPE_SHADER:
		ret = render_import_shader(stream, uuid);
		break;
	case IMPORTTYPE_GLSL_VERTEXSHADER:
		ret = render_import_glsl_vertexshader(stream, uuid);
		break;
	case IMPORTTYPE_GLSL_PIXELSHADER:
		ret = render_import_glsl_pixelshader(stream, uuid);
		break;
	default:
		return -1;
	}

	return ret;
}

#else

int
render_import(stream_t* stream, const uuid_t uuid) {
	FOUNDATION_UNUSED(stream);
	FOUNDATION_UNUSED(uuid);
	return -1;
}

#endif
