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
#include <blake3/blake3.h>

#if RESOURCE_ENABLE_LOCAL_SOURCE

typedef enum { IMPORTTYPE_UNKNOWN, IMPORTTYPE_SHADER, IMPORTTYPE_METAL_SHADER, IMPORTTYPE_VULKAN_SHADER } renderimport_type_t;

static resource_platform_t
render_import_parse_target(const char* target, size_t length, resource_platform_t base) {
	resource_platform_t platform = base;
	// TODO: Generalize
	if (string_equal(target, length, STRING_CONST("metal"))) {
		platform.render_api_group = RENDERAPIGROUP_METAL;
	} else if (string_equal(target, length, STRING_CONST("hlsl"))) {
		platform.render_api_group = RENDERAPIGROUP_VULKAN;
	}
	return platform;
}

static renderimport_type_t
render_import_shader_guess_type(stream_t* stream, renderimport_type_t guess) {
	FOUNDATION_UNUSED(guess);
	renderimport_type_t type = IMPORTTYPE_UNKNOWN;
	char buffer[1024];
	while (!stream_eos(stream)) {
		string_t line = stream_read_line_buffer(stream, buffer, sizeof(buffer), '\n');
		if ((string_find_string(STRING_ARGS(line), STRING_CONST("metal_stdlib"), 0) != STRING_NPOS) ||
		    (string_find_string(STRING_ARGS(line), STRING_CONST("__METAL"), 0) != STRING_NPOS) ||
		    (string_find_string(STRING_ARGS(line), STRING_CONST("namespace metal"), 0) != STRING_NPOS)) {
			type = IMPORTTYPE_METAL_SHADER;
			break;
		} else if ((string_find_string(STRING_ARGS(line), STRING_CONST("VSMain("), 0) != STRING_NPOS) ||
		           (string_find_string(STRING_ARGS(line), STRING_CONST("PSMain("), 0) != STRING_NPOS)) {
			type = IMPORTTYPE_VULKAN_SHADER;
			break;
		}
	}

	stream_seek(stream, 0, STREAM_SEEK_BEGIN);

	return type;
}

static int
render_import_metal_shader(stream_t* stream, const uuid_t uuid) {
	resource_source_t source;
	void* blob = 0;
	size_t size;
	hash_t checksum;
	tick_t timestamp;
	size_t parameter = 0;
	string_const_t* token = 0;
	string_const_t valstr;
	resource_platform_t platformdecl = {-1, -1, RENDERAPIGROUP_METAL, -1, -1, -1};
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

	valstr = string_from_uint_static(parameter, false, 0, 0);
	resource_source_set(&source, timestamp, HASH_PARAMETER_COUNT, platform, STRING_ARGS(valstr));
	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE, 0, STRING_CONST("shader_metal"));

	if (!resource_source_write(&source, uuid, false)) {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Failed writing imported Metal shader: %.*s"),
		          STRING_FORMAT(uuidstr));
		ret = -1;
		goto finalize;
	} else {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_infof(HASH_RESOURCE, STRING_CONST("Wrote imported Metal shader: %.*s"), STRING_FORMAT(uuidstr));
	}

finalize:
	memory_deallocate(blob);
	memory_deallocate(token);
	resource_source_finalize(&source);

	return ret;
}

static int
render_import_vulkan_shader(stream_t* stream, const uuid_t uuid) {
	resource_source_t source;
	void* blob = 0;
	size_t size;
	hash_t checksum;
	tick_t timestamp;
	size_t parameter = 0;
	string_const_t* token = 0;
	string_const_t valstr;
	resource_platform_t platformdecl = {-1, -1, RENDERAPIGROUP_VULKAN, -1, -1, -1};
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

	valstr = string_from_uint_static(parameter, false, 0, 0);
	resource_source_set(&source, timestamp, HASH_PARAMETER_COUNT, platform, STRING_ARGS(valstr));
	resource_source_set(&source, timestamp, HASH_RESOURCE_TYPE, 0, STRING_CONST("shader_vulkan"));

	if (!resource_source_write(&source, uuid, false)) {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_warnf(HASH_RESOURCE, WARNING_SUSPICIOUS, STRING_CONST("Failed writing imported Vulkan shader: %.*s"),
		          STRING_FORMAT(uuidstr));
		ret = -1;
		goto finalize;
	} else {
		string_const_t uuidstr = string_from_uuid_static(uuid);
		log_infof(HASH_RESOURCE, STRING_CONST("Wrote imported Vulkan shader: %.*s"), STRING_FORMAT(uuidstr));
	}

finalize:
	memory_deallocate(blob);
	memory_deallocate(token);
	resource_source_finalize(&source);

	return ret;
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
	if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("shader"))) {
		guess = IMPORTTYPE_SHADER;
	} else if (string_equal_nocase(STRING_ARGS(extension), STRING_CONST("metal"))) {
		guess = IMPORTTYPE_METAL_SHADER;
	}

	type = render_import_shader_guess_type(stream, guess);

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
		uuid_t founduuid = resource_import_map_store(STRING_ARGS(path), uuid, blake3_hash_null());
		if (uuid_is_null(founduuid)) {
			log_warn(HASH_RESOURCE, WARNING_SUSPICIOUS,
			         STRING_CONST("Unable to open import map file to store new resource"));
			return -1;
		}
		uuid = founduuid;
	}

	switch (type) {
		case IMPORTTYPE_SHADER:
			ret = render_import_shader(stream, uuid);
			break;
		case IMPORTTYPE_METAL_SHADER:
			ret = render_import_metal_shader(stream, uuid);
			break;
		case IMPORTTYPE_VULKAN_SHADER:
			ret = render_import_vulkan_shader(stream, uuid);
			break;
		case IMPORTTYPE_UNKNOWN:
		default:
			return -1;
	}

	if (ret == 0)
		resource_import_map_store(STRING_ARGS(path), uuid, blake3_hash_stream(stream));

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
