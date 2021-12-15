/* import.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
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

static resource_platform_t
render_import_parse_target(const char* target, size_t length, resource_platform_t base) {
	resource_platform_t platform = base;
	// TODO: Generalize
	if (string_equal(target, length, STRING_CONST("glsl"))) {
		platform.render_api_group = RENDERAPIGROUP_OPENGL;
	}
	return platform;
}

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
		} else if (string_find_string(STRING_ARGS(line), STRING_CONST("gl_Position"), 0) != STRING_NPOS) {
			type = IMPORTTYPE_GLSL_VERTEXSHADER;
			break;
		}
	}

	stream_seek(stream, 0, STREAM_SEEK_BEGIN);

	return type;
}

static int
glsl_type_to_parameter_type(const string_const_t type) {
	if ((type.length >= 4) && string_equal(type.str, 4, STRING_CONST("vec4")))
		return RENDERPARAMETER_FLOAT4;
	else if ((type.length >= 5) &&
	         (string_equal(type.str, 5, STRING_CONST("ivec4")) || string_equal(type.str, 5, STRING_CONST("uvec4"))))
		return RENDERPARAMETER_INT4;
	else if ((type.length >= 4) && string_equal(type.str, 4, STRING_CONST("mat4")))
		return RENDERPARAMETER_MATRIX;
	else if ((type.length >= 7) && string_equal(type.str, 7, STRING_CONST("sampler")))
		return RENDERPARAMETER_TEXTURE;
	else if ((type.length >= 8) && string_equal(type.str + 1, 7, STRING_CONST("sampler")))
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
	size_t begin;
	size_t next;
	hash_t checksum;
	tick_t timestamp;
	size_t maxtokens;
	size_t parameter;
	string_const_t* token = 0;
	string_const_t valstr;
	char buffer[128];
	resource_platform_t platformdecl = {-1, -1, RENDERAPIGROUP_OPENGL, -1, -1, -1};
	uint64_t platform;
	int ret = 0;

	resource_source_initialize(&source);
	resource_source_read(&source, uuid);

	size = stream_size(stream);
	blob = memory_allocate(HASH_RESOURCE, size, 0, MEMORY_PERSISTENT);

	size = stream_read(stream, blob, size);

	platform = resource_platform(platformdecl);
	timestamp = stream_last_modified(stream);
	checksum = hash(blob, size);
	if (resource_source_write_blob(uuid, timestamp, HASH_SOURCE, platform, checksum, blob, size)) {
		resource_source_set_blob(&source, timestamp, HASH_SOURCE, platform, checksum, size);
	} else {
		ret = -1;
		goto finalize;
	}

	// Parse source and set parameters
	maxtokens = 256;
	token = memory_allocate(HASH_RESOURCE, sizeof(string_const_t) * maxtokens, 0, MEMORY_PERSISTENT);
	begin = 0;
	parameter = 0;
	do {
		string_const_t tokens[64], line;
		size_t itok, ntokens;

		next = string_find_first_of(blob, size, STRING_CONST("\n\r"), begin);
		line = string_substr(blob, size, begin, next - begin);
		ntokens = string_explode(STRING_ARGS(line), STRING_CONST(GLSL_TOKEN_DELIM), tokens, maxtokens, false);

		for (itok = 0; itok < ntokens; ++itok) {
			if ((string_equal(STRING_ARGS(tokens[itok]), STRING_CONST("attribute")) ||
			     string_equal(STRING_ARGS(tokens[itok]), STRING_CONST("uniform"))) &&
			    (itok + 2 < ntokens)) {
				char typebuf[16], dimbuf[16];
				string_const_t typestr = tokens[itok + 1];
				string_const_t namestr = tokens[itok + 2];
				string_const_t dimstr;

				int parameter_type = glsl_type_to_parameter_type(typestr);
				if (parameter_type < 0)
					continue;

				int parameter_dim = glsl_dim_from_token(typestr);
				if (parameter_dim == 1)
					parameter_dim = glsl_dim_from_token(namestr);

				namestr = glsl_name_from_token(namestr);
				typestr = string_to_const(
				    string_from_uint(typebuf, sizeof(typebuf), (unsigned int)parameter_type, false, 0, 0));
				dimstr =
				    string_to_const(string_from_uint(dimbuf, sizeof(dimbuf), (unsigned int)parameter_dim, false, 0, 0));

				log_debugf(HASH_RESOURCE, STRING_CONST("parameter: %.*s type %.*s dim %.*s"), STRING_FORMAT(namestr),
				           STRING_FORMAT(typestr), STRING_FORMAT(dimstr));

				string_t param =
				    string_format(buffer, sizeof(buffer), STRING_CONST("parameter_type_%" PRIsize), parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)), platform, STRING_ARGS(typestr));

				param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_name_%" PRIsize), parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)), platform, STRING_ARGS(namestr));

				param = string_format(buffer, sizeof(buffer), STRING_CONST("parameter_dim_%" PRIsize), parameter);
				resource_source_set(&source, timestamp, hash(STRING_ARGS(param)), platform, STRING_ARGS(dimstr));
				++parameter;
			}
		}

		begin = string_find_first_not_of(blob, size, STRING_CONST(STRING_WHITESPACE), next);
	} while (next != STRING_NPOS);

	valstr = string_from_uint_static(parameter, false, 0, 0);
	resource_source_set(&source, timestamp, HASH_PARAMETER_COUNT, platform, STRING_ARGS(valstr));

	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE, 0, type, type_length);

	if (!resource_source_write(&source, uuid, false)) {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Failed writing imported GLSL shader: %.*s"),
		          STRING_FORMAT(uuidstr));
		ret = -1;
		goto finalize;
	} else {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_infof(HASH_RESOURCE, STRING_CONST("Wrote imported GLSL shader: %.*s"), STRING_FORMAT(uuidstr));
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
	resource_source_t source;
	tick_t timestamp;
	int ret = 0;

	resource_source_initialize(&source);
	resource_source_read(&source, uuid);

	timestamp = stream_last_modified(stream);

	while (!stream_eos(stream)) {
		uuid_t shaderuuid;
		string_const_t target, ref, fullpath;
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		string_split(STRING_ARGS(line), STRING_CONST(" \t"), &target, &ref, false);

		ref = string_strip(STRING_ARGS(ref), STRING_CONST(STRING_WHITESPACE));
		shaderuuid = string_to_uuid(STRING_ARGS(ref));
		if (uuid_is_null(shaderuuid)) {
			if (path_is_absolute(STRING_ARGS(ref))) {
				fullpath = ref;
			} else {
				string_t full;
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				full = path_concat(pathbuf, sizeof(pathbuf), STRING_ARGS(path), STRING_ARGS(ref));
				full = path_absolute(STRING_ARGS(full), sizeof(pathbuf));
				fullpath = string_const(STRING_ARGS(full));
			}

			resource_signature_t sig = resource_import_lookup(STRING_ARGS(fullpath));
			if (uuid_is_null(sig.uuid)) {
				if (!resource_import(STRING_ARGS(fullpath), uuid_null())) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Unable to import linked shader: %.*s"),
					          STRING_FORMAT(fullpath));
					ret = -1;
					goto finalize;
				}
				sig = resource_import_lookup(STRING_ARGS(fullpath));
				if (uuid_is_null(sig.uuid)) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS,
					          STRING_CONST("Import linked shader gave no UUID: %.*s"), STRING_FORMAT(fullpath));
					ret = -1;
					goto finalize;
				}
			}
			shaderuuid = sig.uuid;
		}

		if (!uuid_is_null(shaderuuid)) {
			resource_platform_t platformdecl = {-1, -1, -1, -1, -1, -1};
			resource_platform_t targetplatformdecl = render_import_parse_target(STRING_ARGS(target), platformdecl);
			uint64_t targetplatform = resource_platform(targetplatformdecl);

			string_const_t uuidstr = string_from_uuid_static(shaderuuid);
			resource_source_set(&source, timestamp, HASH_SHADER, targetplatform, STRING_ARGS(uuidstr));

			resource_dependency_t dep;
			dep.uuid = shaderuuid;
			dep.platform = targetplatform;
			resource_source_set_dependencies(uuid, targetplatform, &dep, 1);
		}
	}

	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE, 0, STRING_CONST("shader"));

	if (!resource_source_write(&source, uuid, false)) {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Failed writing imported shader: %.*s"),
		          STRING_FORMAT(uuidstr));
		ret = -1;
		goto finalize;
	} else {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_infof(HASH_RESOURCE, STRING_CONST("Wrote imported shader: %.*s"), STRING_FORMAT(uuidstr));
	}

finalize:
	resource_source_finalize(&source);

	return ret;
}

static int
render_import_program(stream_t* stream, const uuid_t uuid) {
	char buffer[1024];
	char pathbuf[BUILD_MAX_PATHLEN];
	resource_source_t source;
	resource_platform_t platformdecl = {-1, -1, -1, -1, -1, -1};
	uint64_t platform;
	tick_t timestamp;
	int ret = 0;

	resource_source_initialize(&source);
	resource_source_read(&source, uuid);

	platform = resource_platform(platformdecl);
	timestamp = stream_last_modified(stream);

	resource_dependency_t* dependencies = nullptr;

	while (!stream_eos(stream)) {
		string_const_t type, ref;
		string_const_t fullpath;
		string_const_t uuidstr;
		uuid_t shaderuuid;
		hash_t typehash;
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		string_split(STRING_ARGS(line), STRING_CONST(" \t"), &type, &ref, false);

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
			} else {
				string_t full;
				string_const_t path = stream_path(stream);
				path = path_directory_name(STRING_ARGS(path));
				full = path_concat(pathbuf, sizeof(pathbuf), STRING_ARGS(path), STRING_ARGS(ref));
				full = path_absolute(STRING_ARGS(full), sizeof(pathbuf));
				fullpath = string_const(STRING_ARGS(full));
			}

			resource_signature_t sig = resource_import_lookup(STRING_ARGS(fullpath));
			if (uuid_is_null(sig.uuid)) {
				if (!resource_import(STRING_ARGS(fullpath), uuid_null())) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Unable to import linked shader: %.*s"),
					          STRING_FORMAT(fullpath));
					ret = -1;
					goto finalize;
				}
				sig = resource_import_lookup(STRING_ARGS(fullpath));
				if (uuid_is_null(sig.uuid)) {
					log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS,
					          STRING_CONST("Import linked shader gave no UUID: %.*s"), STRING_FORMAT(fullpath));
					ret = -1;
					goto finalize;
				}
			}
			shaderuuid = sig.uuid;
		}

		if (!uuid_is_null(shaderuuid)) {
			uuidstr = string_from_uuid_static(shaderuuid);
			resource_source_set(&source, timestamp, typehash, platform, STRING_ARGS(uuidstr));
			resource_dependency_t dep;
			dep.uuid = shaderuuid;
			dep.platform = platform;
			array_push(dependencies, dep);
		}
	}

	resource_source_set_dependencies(uuid, platform, dependencies, array_size(dependencies));

	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE, 0, STRING_CONST("program"));

	if (!resource_source_write(&source, uuid, false)) {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Failed writing imported program: %.*s"),
		          STRING_FORMAT(uuidstr));
		ret = -1;
		goto finalize;
	} else {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_infof(HASH_RESOURCE, STRING_CONST("Wrote imported program: %.*s"), STRING_FORMAT(uuidstr));
	}

finalize:
	resource_source_finalize(&source);
	array_deallocate(dependencies);

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
	else if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("glsl"))) {
		guess = IMPORTTYPE_SHADER;
		string_const_t basename = path_base_file_name(STRING_ARGS(path));
		string_const_t subextension = path_file_extension(STRING_ARGS(basename));
		if (string_equal_nocase(STRING_ARGS(subextension), STRING_CONST("pixel")))
			guess = IMPORTTYPE_GLSL_PIXELSHADER;
		else if (string_equal_nocase(STRING_ARGS(subextension), STRING_CONST("vertex")))
			guess = IMPORTTYPE_GLSL_VERTEXSHADER;
	}

	type = render_import_shader_guess_type(stream);

	if ((type == IMPORTTYPE_UNKNOWN) && (guess != IMPORTTYPE_UNKNOWN))
		type = guess;

	if (type == IMPORTTYPE_UNKNOWN)
		return -1;

	if (uuid_is_null(uuid))
		uuid = resource_import_lookup(STRING_ARGS(path)).uuid;

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
		case IMPORTTYPE_UNKNOWN:
		default:
			return -1;
	}

	if (ret == 0)
		resource_import_map_store(STRING_ARGS(path), uuid, stream_sha256(stream));

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
