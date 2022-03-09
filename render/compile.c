/* compile.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson
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
#include <window/window.h>

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic push
#if __has_warning("-Wdeprecated-declarations")
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

#if RESOURCE_ENABLE_LOCAL_SOURCE

int
render_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const uint256_t source_hash,
               const char* type, size_t type_length) {
	if (render_shader_compile(uuid, platform, source, source_hash, type, type_length) == 0)
		return 0;
	return -1;
}

static resource_change_t*
render_resource_source_platform_reduce(resource_change_t* change, resource_change_t* best, void* data) {
	uint64_t** subplatforms = data;
	uint64_t platform = (*subplatforms)[0];
	size_t iplat, psize;
	FOUNDATION_UNUSED(best);
	if ((platform == RESOURCE_PLATFORM_ALL) ||
	    resource_platform_is_equal_or_more_specific(change->platform, platform)) {
		for (iplat = 1, psize = array_size(*subplatforms); iplat != psize; ++iplat) {
			if ((*subplatforms)[iplat] == change->platform)
				break;
		}
		if (iplat >= psize)
			array_push(*subplatforms, change->platform);
	}
	return 0;
}

static resource_change_t*
render_resource_source_platform_super(resource_change_t* change, resource_change_t* best, void* data) {
	uint64_t** subplatforms = data;
	uint64_t platform = (*subplatforms)[0];
	FOUNDATION_UNUSED(best);
	if (platform == RESOURCE_PLATFORM_ALL) {
		array_push(*subplatforms, platform);
		return (void*)((uintptr_t)-1);
	}
	if (resource_platform_is_equal_or_more_specific(platform, change->platform)) {
		if (array_size(*subplatforms) == 1) {
			array_push(*subplatforms, change->platform);
		} else if (resource_platform_is_equal_or_more_specific(change->platform, (*subplatforms)[1])) {
			(*subplatforms)[1] = change->platform;
		}
	}
	return 0;
}

static bool
render_shader_shader_resource_type_valid(hash_t type) {
	if (type == HASH_SHADER_METAL)
		return true;
	return false;
}

static int
render_shader_ref_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const uint256_t source_hash,
                          const char* type, size_t type_length) {
	FOUNDATION_UNUSED(type);
	FOUNDATION_UNUSED(type_length);

	// Defer compilation to target shader and copy successful result
	int result = -1;
	bool recompile = false;
	bool recompiled = false;
	stream_t* ressource;
	stream_t* restarget;

	char uuidbuf[40];
	const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid);
	error_context_push(STRING_CONST("compiling shader"), STRING_ARGS(uuidstr));

	log_debugf(HASH_RENDER, STRING_CONST("Compiling shader ref: %.*s platform 0x%" PRIx64), STRING_FORMAT(uuidstr),
	           platform);
	resource_change_t* shaderchange = resource_source_get(source, HASH_SHADER, platform);
	uuid_t shaderuuid = shaderchange ? string_to_uuid(STRING_ARGS(shaderchange->value.value)) : uuid_null();
	if (uuid_is_null(shaderuuid)) {
		error_context_pop();
		return -1;
	}
	if (resource_compile_need_update(shaderuuid, platform)) {
		recompiled = resource_compile(shaderuuid, platform);
		if (!recompiled) {
			error_context_pop();
			return -1;
		}
	}

retry:

	ressource = resource_local_open_static(shaderuuid, platform);
	restarget = resource_local_create_static(uuid, platform);
	if (ressource && restarget) {
		size_t source_size = stream_size(ressource);

		resource_header_t header = resource_stream_read_header(ressource);
		if (!render_shader_shader_resource_type_valid(header.type) ||
		    (header.version != RENDER_SHADER_RESOURCE_VERSION)) {
			recompile = true;
		} else {
			source_size -= stream_tell(ressource);

			header.source_hash = source_hash;
			resource_stream_write_header(restarget, header);

			char* buffer = memory_allocate(HASH_RESOURCE, source_size, 0, MEMORY_PERSISTENT);
			if (stream_read(ressource, buffer, source_size) == source_size) {
				if (stream_write(restarget, buffer, source_size) == source_size)
					result = 0;
			}
			memory_deallocate(buffer);
		}
	}
	stream_deallocate(restarget);
	stream_deallocate(ressource);

	if (result == 0) {
		result = -1;
		ressource = resource_local_open_dynamic(shaderuuid, platform);
		restarget = resource_local_create_dynamic(uuid, platform);
		if (ressource && restarget) {
			size_t source_size = stream_size(ressource);
			char* buffer = memory_allocate(HASH_RESOURCE, source_size, 0, MEMORY_PERSISTENT);
			if (stream_read(ressource, buffer, source_size) == source_size) {
				if (stream_write(restarget, buffer, source_size) == source_size)
					result = 0;
			} else {
				recompile = true;
			}
			memory_deallocate(buffer);
		} else {
			recompile = true;
		}
		stream_deallocate(restarget);
		stream_deallocate(ressource);
	}

	if (recompile && !recompiled) {
		recompiled = resource_compile(shaderuuid, platform);
		if (recompiled)
			goto retry;
	}

	error_context_pop();

	return result;
}

int
render_shader_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const uint256_t source_hash,
                      const char* type, size_t type_length) {
	int result = 0;
	uint64_t* subplatforms = 0;
	size_t iplat, psize;
	hashmap_fixed_t fixedmap;
	hashmap_t* map = (hashmap_t*)&fixedmap;
	resource_platform_t platform_decl;
	render_backend_t* backend = 0;
	window_t window;
	hash_t resource_type_hash;

	resource_type_hash = hash(type, type_length);

	if (resource_type_hash == HASH_SHADER)
		return render_shader_ref_compile(uuid, platform, source, source_hash, type, type_length);

	if (!render_shader_shader_resource_type_valid(resource_type_hash))
		return -1;

	error_context_declare_local(char uuidbuf[40];
	                            const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid));
	error_context_push(STRING_CONST("compiling shader"), STRING_ARGS(uuidstr));

	array_push(subplatforms, platform);

	hashmap_initialize(map, sizeof(fixedmap.bucket) / sizeof(fixedmap.bucket[0]), 8);
	resource_source_map_all(source, map, false);
	resource_source_map_reduce(source, map, &subplatforms, render_resource_source_platform_reduce);
	resource_source_map_clear(map);
	if (array_size(subplatforms) == 1) {
		// The requested platform had no values, find most specialized platform
		// which is a super of the requested platform
		resource_source_map_all(source, map, false);
		resource_source_map_reduce(source, map, &subplatforms, render_resource_source_platform_super);
		resource_source_map_clear(map);
	}
	hashmap_finalize(map);

	render_backend_t* prev_backend = render_backend_thread();

	bool valid_platform = false;
	for (iplat = 1, psize = array_size(subplatforms); (iplat != psize) && (result == 0); ++iplat) {
		void* compiled_blob = 0;
		size_t compiled_size = 0;
		stream_t* stream;
		uint64_t subplatform = subplatforms[iplat];
		if (subplatform == 0)
			continue;  // Shaders are always platform specific

		platform_decl = resource_platform_decompose(subplatform);
		if (platform_decl.render_api <= RENDERAPI_DEFAULT) {
			if (platform_decl.render_api_group == RENDERAPIGROUP_METAL) {
				platform_decl.render_api = RENDERAPI_METAL;
				if (prev_backend && (prev_backend->api == RENDERAPI_METAL))
					backend = prev_backend;
			} else {
				continue;  // Nonspecific render api
			}
		}

		valid_platform = true;
		if (!backend)
			backend = render_backend_allocate((render_api_t)platform_decl.render_api, true);
		if (!backend) {
			log_warn(HASH_RESOURCE, WARNING_UNSUPPORTED, STRING_CONST("Unable to create render backend"));
			result = -1;
			continue;
		}

		render_target_t* render_target = nullptr;
		if (backend != prev_backend) {
#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
			window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render compile"), 100, 100,
			              WINDOW_FLAG_NOSHOW);
#else
			window_initialize(&window, nullptr);
#endif
			render_target = render_target_window_allocate(backend, &window, 0);
		}

		result = -1;
#if FOUNDATION_PLATFORM_APPLE
		if (platform_decl.render_api == RENDERAPI_METAL) {
			char* sourcebuffer = 0;
			resource_change_t* sourcechange = resource_source_get(source, HASH_SOURCE, subplatform);
			if (sourcechange && (sourcechange->flags & RESOURCE_SOURCEFLAG_BLOB)) {
				sourcebuffer = memory_allocate(HASH_RESOURCE, sourcechange->value.blob.size, 0, MEMORY_PERSISTENT);
				if (!resource_source_read_blob(uuid, HASH_SOURCE, subplatform, sourcechange->value.blob.checksum,
				                               sourcebuffer, sourcechange->value.blob.size)) {
					log_error(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Failed to read full source blob"));
					memory_deallocate(sourcebuffer);
					sourcebuffer = 0;
				}
			}
			if (sourcebuffer) {
				// Write to a temporary file and use command line tooling to generate the binary blob
				char pathbuf[BUILD_MAX_PATHLEN];
				string_t source_file = path_make_temporary(pathbuf, sizeof(pathbuf));
				source_file = string_append(STRING_ARGS(source_file), sizeof(pathbuf), STRING_CONST(".metal"));
				string_const_t directory = path_directory_name(STRING_ARGS(source_file));
				fs_make_directory(STRING_ARGS(directory));

				stream_t* source_stream = fs_open_file(STRING_ARGS(source_file),
				                                       STREAM_OUT | STREAM_BINARY | STREAM_CREATE | STREAM_TRUNCATE);
				stream_write(source_stream, sourcebuffer, sourcechange->value.blob.size);
				stream_deallocate(source_stream);
				memory_deallocate(sourcebuffer);

				string_t air_file = string_allocate_concat(STRING_ARGS(source_file), STRING_CONST(".air"));
				string_t lib_file = string_allocate_concat(STRING_ARGS(source_file), STRING_CONST(".metallib"));

				process_t* compile_process = process_allocate();

				string_const_t proc_args[7];
				size_t proc_args_count = sizeof(proc_args) / sizeof(proc_args[0]);

				proc_args[0] = string_const(STRING_CONST("-sdk"));
				proc_args[1] = string_const(STRING_CONST("macosx"));
				proc_args[2] = string_const(STRING_CONST("metal"));
				proc_args[3] = string_const(STRING_CONST("-c"));
				proc_args[4] = path_strip_protocol(STRING_ARGS(source_file));
				proc_args[5] = string_const(STRING_CONST("-o"));
				proc_args[6] = path_strip_protocol(STRING_ARGS(air_file));

				log_infof(HASH_RENDER, STRING_CONST("Compiling Metal source: %.*s -> %.*s"),
				          STRING_FORMAT(proc_args[4]), STRING_FORMAT(proc_args[6]));

				process_set_executable_path(compile_process, STRING_CONST("/usr/bin/xcrun"));
				process_set_arguments(compile_process, proc_args, proc_args_count);
				int spawn_ret = process_spawn(compile_process);
				process_deallocate(compile_process);

				if (spawn_ret == 0) {
					process_t* lib_process = process_allocate();

					proc_args[2] = string_const(STRING_CONST("metallib"));
					proc_args[3] = path_strip_protocol(STRING_ARGS(air_file));
					proc_args[4] = string_const(STRING_CONST("-o"));
					proc_args[5] = path_strip_protocol(STRING_ARGS(lib_file));
					proc_args_count = 6;

					log_infof(HASH_RENDER, STRING_CONST("Compiling Metal library: %.*s -> %.*s"),
					          STRING_FORMAT(proc_args[3]), STRING_FORMAT(proc_args[5]));

					process_set_executable_path(lib_process, STRING_CONST("/usr/bin/xcrun"));
					process_set_arguments(lib_process, proc_args, proc_args_count);
					spawn_ret = process_spawn(lib_process);
					process_deallocate(lib_process);

					if (spawn_ret == 0) {
						stream_t* lib_stream = stream_open(STRING_ARGS(lib_file), STREAM_IN | STREAM_BINARY);
						if (lib_stream) {
							compiled_size = stream_size(lib_stream);
							compiled_blob = memory_allocate(HASH_RESOURCE, compiled_size, 0, MEMORY_PERSISTENT);
							stream_read(lib_stream, compiled_blob, compiled_size);
							stream_deallocate(lib_stream);
							result = 0;
						} else {
							log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
							          STRING_CONST("Failed to read compiled Metal lib after compile"));
						}
					} else {
						log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to compile Metal lib: %d"),
						           spawn_ret);
						_exit(-1);
					}
				} else {
					log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to compile Metal source: %d"),
					           spawn_ret);
				}

				string_deallocate(lib_file.str);
				string_deallocate(air_file.str);
			}
		}
#endif
		if (backend != prev_backend) {
			render_target_deallocate(render_target);
			render_backend_deallocate(backend);
			window_finalize(&window);
		}

		if (result < 0)
			continue;

		stream = resource_local_create_static(uuid, subplatform);
		if (stream) {
			uint32_t version = RENDER_SHADER_RESOURCE_VERSION;
			resource_header_t header = {.type = resource_type_hash, .version = version, .source_hash = source_hash};
			render_shader_t shader;
			resource_stream_write_header(stream, header);
			render_shader_initialize(&shader);
			stream_write(stream, &shader, sizeof(shader));
			stream_deallocate(stream);

			if (compiled_size > 0) {
				stream = resource_local_create_dynamic(uuid, subplatform);
				if (stream) {
					stream_write_uint32(stream, version);
					stream_write_uint64(stream, compiled_size);
					stream_write(stream, compiled_blob, compiled_size);
					stream_deallocate(stream);

					result = 0;
				} else {
					log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL,
					           STRING_CONST("Unable to create dynamic resource stream"));
					result = -1;
				}
			}
		} else {
			log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create static resource stream"));
			result = -1;
		}

		memory_deallocate(compiled_blob);
	}

	array_deallocate(subplatforms);

	if (!valid_platform) {
		result = -1;
		log_error(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("Shader has no valid platforms set"));
	}

	error_context_pop();

	return result;
}

#else

int
render_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const uint256_t source_hash,
               const char* type, size_t type_length) {
	FOUNDATION_UNUSED(uuid);
	FOUNDATION_UNUSED(platform);
	FOUNDATION_UNUSED(source);
	FOUNDATION_UNUSED(source_hash);
	FOUNDATION_UNUSED(type);
	FOUNDATION_UNUSED(type_length);
	return -1;
}

#endif

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic pop
#endif
