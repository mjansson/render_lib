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

#if !FOUNDATION_PLATFORM_APPLE
#include "gl4/glwrap.h"
#include "gl4/glprocs.h"
#endif

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
	if (render_program_compile(uuid, platform, source, source_hash, type, type_length) == 0)
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
		if (((header.type != HASH_VERTEXSHADER) && (header.type != HASH_PIXELSHADER)) ||
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

	if ((resource_type_hash != HASH_VERTEXSHADER) && (resource_type_hash != HASH_PIXELSHADER) &&
	    (resource_type_hash != HASH_SHADER))
		return -1;

	if (resource_type_hash == HASH_SHADER)
		return render_shader_ref_compile(uuid, platform, source, source_hash, type, type_length);

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
			if (platform_decl.render_api_group == RENDERAPIGROUP_OPENGL) {
				platform_decl.render_api = RENDERAPI_OPENGL;
			} else if (platform_decl.render_api_group == RENDERAPIGROUP_DIRECTX) {
				platform_decl.render_api = RENDERAPI_DIRECTX;
			} else if (platform_decl.render_api_group == RENDERAPIGROUP_GLES) {
				platform_decl.render_api = RENDERAPI_GLES;
			} else if (platform_decl.render_api_group == RENDERAPIGROUP_METAL) {
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

		render_drawable_t drawable;
		if (backend != prev_backend) {
#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
			window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render compile"), 100, 100,
			              WINDOW_FLAG_NOSHOW);
#else
			window_initialize(&window, nullptr);
#endif
			render_drawable_initialize_window(&drawable, &window, 0);

			render_backend_set_format(backend, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR);
			render_backend_set_drawable(backend, &drawable);
		}

		result = -1;
#if !FOUNDATION_PLATFORM_APPLE
		if ((platform_decl.render_api >= RENDERAPI_OPENGL) && (platform_decl.render_api <= RENDERAPI_OPENGL4)) {
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
				GLuint handle = glCreateShader(string_equal(type, type_length, STRING_CONST("vertexshader")) ?
				                                   GL_VERTEX_SHADER_ARB :
                                                   GL_FRAGMENT_SHADER_ARB);
				const GLchar* glsource = (GLchar*)sourcebuffer;
				GLint source_size = (GLint)sourcechange->value.blob.size;
				glShaderSource(handle, 1, &glsource, &source_size);
				glCompileShader(handle);

				GLsizei log_capacity = 2048;
				GLchar* log_buffer = memory_allocate(HASH_RESOURCE, (size_t)log_capacity, 0, MEMORY_TEMPORARY);
				GLint log_length = 0;
				GLint compiled = 0;
				glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);
				glGetShaderInfoLog(handle, log_capacity, &log_length, log_buffer);
				if (!compiled) {
					log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("GLSL compiler failed: %.*s"),
					           (int)log_length, log_buffer);
					result = -1;
				} else {
					if (log_length < 2) {
						string_t nomsg = string_copy(log_buffer, (size_t)log_capacity, STRING_CONST("<no message>"));
						log_length = (GLint)nomsg.length;
					}
					log_debugf(HASH_RESOURCE, STRING_CONST("Successfully compiled shader: %.*s"), (int)log_length,
					           log_buffer);

					compiled_size = sourcechange->value.blob.size;
					compiled_blob = memory_allocate(HASH_RESOURCE, compiled_size, 0, MEMORY_PERSISTENT);
					memcpy(compiled_blob, sourcebuffer, compiled_size);
				}
				memory_deallocate(log_buffer);
				memory_deallocate(sourcebuffer);
				glDeleteShader(handle);
			}

			if (compiled_size > 0)
				result = 0;
		}
#endif
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
			render_backend_deallocate(backend);
			render_drawable_finalize(&drawable);
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
			if (resource_type_hash == HASH_VERTEXSHADER)
				render_vertexshader_initialize(&shader);
			else
				render_pixelshader_initialize(&shader);
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

	if (prev_backend)
		render_backend_enable_thread(prev_backend);

	array_deallocate(subplatforms);

	if (!valid_platform) {
		result = -1;
		log_error(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("Shader has no valid platforms set"));
	}

	error_context_pop();

	return result;
}

static render_program_t*
render_program_compile_opengl(render_backend_t* backend, uuid_t vertexshader, uuid_t pixelshader) {
	FOUNDATION_UNUSED(backend, vertexshader, pixelshader);
	render_program_t* program = 0;
#if !FOUNDATION_PLATFORM_APPLE
	render_shader_t* vshader = 0;
	render_shader_t* pshader = 0;
	GLuint handle = 0;
	GLsizei log_capacity = 2048;
	GLchar* log_buffer = memory_allocate(HASH_RESOURCE, (size_t)log_capacity, 0, MEMORY_TEMPORARY);
	GLint log_length = 0;
	GLint compiled = 0;

	vshader = render_shader_load(backend, vertexshader);
	pshader = render_shader_load(backend, pixelshader);
	if (!vshader || !(vshader->shadertype & SHADER_VERTEX)) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("Unable to load vertex shader"));
		goto exit;
	}
	if (!pshader || !(pshader->shadertype & SHADER_PIXEL)) {
		log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("Unable to load pixel shader"));
		goto exit;
	}

	char name[256];
	GLint attributes = 0;
	GLint uniforms = 0;
	uint16_t offset = 0;

	handle = glCreateProgram();
	if (!handle) {
		log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create program object"));
		goto exit;
	}

	glAttachShader(handle, (GLuint)vshader->backend_data[0]);
	glAttachShader(handle, (GLuint)pshader->backend_data[0]);
	glLinkProgram(handle);
	glGetProgramiv(handle, GL_LINK_STATUS, &compiled);
	glGetProgramInfoLog(handle, log_capacity, &log_length, log_buffer);

	if (!compiled) {
		log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to link program: %.*s"), (int)log_length,
		           log_buffer);
		goto exit;
	} else {
		if (log_length < 2) {
			string_t nomsg = string_copy(log_buffer, (size_t)log_capacity, STRING_CONST("<no message>"));
			log_length = (GLint)nomsg.length;
		}
		log_debugf(HASH_RESOURCE, STRING_CONST("Successfully linked program: %.*s"), (int)log_length, log_buffer);
	}

	glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &attributes);
	for (GLint ia = 0; ia < attributes; ++ia) {
		GLsizei char_count = 0;
		GLint size = 0;
		GLenum gltype = GL_NONE;
		glGetActiveAttrib(handle, (GLuint)ia, sizeof(name), &char_count, &size, &gltype, name);

		if (char_count) {
			GLuint binding = 0;
			// TODO: Implement something proper, and generalized, together with dx10 input layout
			if (string_equal(name, (size_t)char_count, STRING_CONST("position")))
				binding = VERTEXATTRIBUTE_POSITION;
			else if (string_equal(name, (size_t)char_count, STRING_CONST("color")))
				binding = VERTEXATTRIBUTE_PRIMARYCOLOR;
			else if (string_equal(name, (size_t)char_count, STRING_CONST("normal")))
				binding = VERTEXATTRIBUTE_NORMAL;
			else if (string_equal(name, (size_t)char_count, STRING_CONST("texcoord")))
				binding = VERTEXATTRIBUTE_TEXCOORD0;
			else if (string_equal(name, (size_t)char_count, STRING_CONST("tangent")))
				binding = VERTEXATTRIBUTE_TANGENT;
			else if (string_equal(name, (size_t)char_count, STRING_CONST("bitangent")))
				binding = VERTEXATTRIBUTE_BITANGENT;
			else if ((char_count > 8) && string_equal(name, 8, STRING_CONST("texcoord"))) {
				unsigned int level = string_to_uint(name + 8, (size_t)char_count - 8, false);
				binding = VERTEXATTRIBUTE_TEXCOORD0 + level;
			}
			glBindAttribLocation(handle, binding, name);
		}
	}

	glLinkProgram(handle);
	glGetProgramiv(handle, GL_LINK_STATUS, &compiled);
	glGetProgramInfoLog(handle, log_length, &log_length, log_buffer);

	if (!compiled) {
		log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to relink program: %.*s"),
		           (int)log_length, log_buffer);
		goto exit;
	} else {
		if (log_length < 2) {
			string_t nomsg = string_copy(log_buffer, (size_t)log_capacity, STRING_CONST("<no message>"));
			log_length = (GLint)nomsg.length;
		}
		log_debugf(HASH_RESOURCE, STRING_CONST("Successfully relinked program: %.*s"), (int)log_length, log_buffer);
	}

	glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);

	program = render_program_allocate((size_t)uniforms);
	program->vertexshader = vshader;
	program->pixelshader = pshader;
	program->attributes_count = 0;

	for (GLint ia = 0; (ia < attributes) && compiled; ++ia) {
		GLsizei char_count = 0;
		GLint size = 0;
		GLenum gltype = GL_NONE;

		glGetActiveAttrib(handle, (GLuint)ia, sizeof(name), &char_count, &size, &gltype, name);

		// TODO: Implement something proper, and generalized, together with dx10 input layout
		unsigned int attrib = 0;
		if (string_equal(name, (size_t)char_count, STRING_CONST("position")))
			attrib = VERTEXATTRIBUTE_POSITION;
		else if (string_equal(name, (size_t)char_count, STRING_CONST("normal")))
			attrib = VERTEXATTRIBUTE_NORMAL;
		else if (string_equal(name, (size_t)char_count, STRING_CONST("color")))
			attrib = VERTEXATTRIBUTE_PRIMARYCOLOR;
		else if (string_equal(name, (size_t)char_count, STRING_CONST("texcoord")))
			attrib = VERTEXATTRIBUTE_TEXCOORD0;
		else if (string_equal(name, (size_t)char_count, STRING_CONST("tangent")))
			attrib = VERTEXATTRIBUTE_TANGENT;
		else if (string_equal(name, (size_t)char_count, STRING_CONST("bitangent")))
			attrib = VERTEXATTRIBUTE_BITANGENT;
		else if ((char_count > 8) && string_equal(name, 8, STRING_CONST("texcoord"))) {
			unsigned int level = string_to_uint(name + 8, (size_t)char_count - 8, false);
			attrib = VERTEXATTRIBUTE_TEXCOORD0 + level;
		} else {
			log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Invalid/unknown attribute name: %.*s"),
			           (int)char_count, name);
			compiled = false;
			break;
		}

		render_program_attribute_t* program_attribute = program->attribute + program->attributes_count;
		program_attribute->binding = (uint16_t)attrib;

		switch (gltype) {
			case GL_FLOAT:
				program_attribute->format = VERTEXFORMAT_FLOAT;
				break;
			case GL_FLOAT_VEC2:
				program_attribute->format = VERTEXFORMAT_FLOAT2;
				break;
			case GL_FLOAT_VEC3:
				program_attribute->format = VERTEXFORMAT_FLOAT3;
				break;
			case GL_FLOAT_VEC4:
				program_attribute->format = VERTEXFORMAT_FLOAT4;
				break;
			case GL_INT:
			case GL_UNSIGNED_INT:
				program_attribute->format = VERTEXFORMAT_INT;
				break;
			case GL_INT_VEC2:
			case 0x8DC6:  // GL_UNSIGNED_INT_VEC2:
				program_attribute->format = VERTEXFORMAT_INT2;
				break;
			case GL_INT_VEC4:
			case 0x8DC8:  // GL_UNSIGNED_INT_VEC4:
				program_attribute->format = VERTEXFORMAT_INT4;
				break;
			default:
				log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Invalid/unknown attribute type: %u"),
				           (unsigned int)gltype);
				compiled = false;
				break;
		}
		program->attribute_name[program->attributes_count] = hash(name, (size_t)char_count);

		++program->attributes_count;
	}
	if (!compiled)
		goto exit;

	offset = 0;
	for (GLint iu = 0; (iu < uniforms) && compiled; ++iu) {
		GLsizei char_count = 0;
		GLint size = 0;
		GLenum gltype = GL_NONE;
		render_parameter_t* parameter = program->parameters + iu;

		glGetActiveUniform(handle, (GLuint)iu, sizeof(name), &char_count, &size, &gltype, name);
		size_t bracket_pos = string_find(name, (size_t)char_count, '[', 0);
		if (bracket_pos != STRING_NPOS) {
			name[bracket_pos] = 0;
			char_count = (GLsizei)bracket_pos;
		}

		parameter->name = hash(name, (size_t)char_count);
		parameter->location = (unsigned int)glGetUniformLocation(handle, name);
		parameter->dim = (uint16_t)size;
		parameter->offset = offset;
		parameter->stages = SHADER_VERTEX | SHADER_PIXEL;

		switch (gltype) {
			case GL_FLOAT:
				parameter->type = RENDERPARAMETER_FLOAT;
				offset += (uint16_t)(4 * size);
				break;
			case GL_FLOAT_VEC4:
				parameter->type = RENDERPARAMETER_FLOAT4;
				offset += (uint16_t)(16 * size);
				break;
			case GL_INT_VEC4:
			case 0x8DC8:  // GL_UNSIGNED_INT_VEC4:
				parameter->type = RENDERPARAMETER_INT4;
				offset += (uint16_t)(16 * size);
				break;
			case GL_FLOAT_MAT4:
				parameter->type = RENDERPARAMETER_MATRIX;
				offset += (uint16_t)(16 * 4 * size);
				break;
			case GL_SAMPLER_2D:
				parameter->type = RENDERPARAMETER_TEXTURE;
				offset += (uint16_t)(sizeof(GLuint) * (size_t)size);
				break;
			default:
				log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Invalid/unknown uniform type: %u"),
				           (unsigned int)gltype);
				compiled = false;
				break;
		}
	}

	program->size_parameterdata = offset;

exit:
	if (!compiled) {
		if (program) {
			render_program_deallocate(program);
			program = 0;
		} else {
			if (vshader)
				render_shader_unload(vshader);
			if (pshader)
				render_shader_unload(pshader);
		}
	}

	if (handle)
		glDeleteProgram(handle);

	memory_deallocate(log_buffer);
#endif
	return program;
}

static render_program_t*
render_program_compile_metal(render_backend_t* backend, uuid_t vertexshader, uuid_t pixelshader) {
	FOUNDATION_UNUSED(backend, vertexshader, pixelshader);
	render_program_t* program = 0;
#if FOUNDATION_PLATFORM_APPLE
	program = render_program_allocate(0);
	program->vertexshader = 0;
	program->pixelshader = 0;
	program->attributes_count = 0;
#endif
	return program;
}

int
render_program_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source, const uint256_t source_hash,
                       const char* type, size_t type_length) {
	int result = -1;
	hash_t resource_type_hash;
	uint64_t* subplatforms = 0;
	uint64_t* moreplatforms = 0;
	size_t iplat, psize;
	size_t imore, moresize;
	bool superplatform = false;
	hashmap_fixed_t fixedmap;
	hashmap_t* map = (hashmap_t*)&fixedmap;
	resource_platform_t platform_decl;
	render_backend_t* backend = 0;
	window_t window;

	resource_type_hash = hash(type, type_length);

	if (resource_type_hash != HASH_PROGRAM)
		return result;

	error_context_declare_local(char uuidbuf[40];
	                            const string_t uuidstr = string_from_uuid(uuidbuf, sizeof(uuidbuf), uuid));
	error_context_push(STRING_CONST("compiling program"), STRING_ARGS(uuidstr));

	array_push(subplatforms, platform);

	hashmap_initialize(map, sizeof(fixedmap.bucket) / sizeof(fixedmap.bucket[0]), 8);
	resource_source_map_all(source, map, false);
	resource_source_map_reduce(source, map, &subplatforms, render_resource_source_platform_reduce);
	resource_source_map_clear(map);
	if (array_size(subplatforms) == 1) {
		// The requested platform had no values, find most specialized platform
		// which is a super of the requested platform
		superplatform = true;
		resource_source_map_all(source, map, false);
		resource_source_map_reduce(source, map, &subplatforms, render_resource_source_platform_super);
		resource_source_map_clear(map);
	}

	// First make sure we catch specialized platforms from shaders since
	// programs are the sum of the shaders
	for (iplat = 1, psize = array_size(subplatforms); iplat != psize; ++iplat) {
		uuid_t vertexshader = uuid_null();
		uuid_t pixelshader = uuid_null();
		uint64_t subplatform = subplatforms[iplat];

		resource_source_t shadersource;
		uint64_t* shaderplatforms = 0;
		size_t ishaderplat, pshadersize;

		resource_change_t* shaderchange = resource_source_get(source, HASH_VERTEXSHADER, subplatform);
		if (shaderchange && (shaderchange->flags & RESOURCE_SOURCEFLAG_VALUE))
			vertexshader = string_to_uuid(STRING_ARGS(shaderchange->value.value));
		if (uuid_is_null(vertexshader)) {
			log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE,
			           STRING_CONST("Missing vertex shader for platform 0x%" PRIx64), subplatform);
			subplatforms[iplat] = 0;
			continue;
		}

		shaderchange = resource_source_get(source, HASH_PIXELSHADER, subplatform);
		if (shaderchange && (shaderchange->flags & RESOURCE_SOURCEFLAG_VALUE))
			pixelshader = string_to_uuid(STRING_ARGS(shaderchange->value.value));
		if (uuid_is_null(pixelshader)) {
			log_errorf(HASH_RESOURCE, ERROR_INVALID_VALUE, STRING_CONST("Missing pixel shader for platform 0x%" PRIx64),
			           subplatform);
			subplatforms[iplat] = 0;
			continue;
		}

		// Get additional more specialized platform targets from shaders
		array_push(shaderplatforms, subplatform);

		if (resource_autoimport_need_update(vertexshader, subplatform) ||
		    resource_autoimport_need_update(vertexshader, platform)) {
			string_const_t vsuuidstr = string_from_uuid_static(vertexshader);
			log_debugf(HASH_RESOURCE, STRING_CONST("Reimporting vertex shader resource %.*s"),
			           STRING_FORMAT(vsuuidstr));
			resource_autoimport(vertexshader);
		}

		if (resource_autoimport_need_update(pixelshader, subplatform) ||
		    resource_autoimport_need_update(pixelshader, platform)) {
			string_const_t psuuidstr = string_from_uuid_static(pixelshader);
			log_debugf(HASH_RESOURCE, STRING_CONST("Reimporting pixel shader resource %.*s"), STRING_FORMAT(psuuidstr));
			resource_autoimport(pixelshader);
		}

		resource_source_initialize(&shadersource);
		if (resource_source_read(&shadersource, vertexshader)) {
			resource_source_map_all(&shadersource, map, false);
			resource_source_map_reduce(&shadersource, map, &shaderplatforms, render_resource_source_platform_reduce);
			resource_source_map_clear(map);
		}
		resource_source_finalize(&shadersource);

		resource_source_initialize(&shadersource);
		if (resource_source_read(&shadersource, pixelshader)) {
			resource_source_map_all(&shadersource, map, false);
			resource_source_map_reduce(&shadersource, map, &shaderplatforms, render_resource_source_platform_reduce);
			resource_source_map_clear(map);
		}
		resource_source_finalize(&shadersource);

		for (ishaderplat = 1, pshadersize = array_size(shaderplatforms); ishaderplat != pshadersize; ++ishaderplat) {
			uint64_t moreplatform = shaderplatforms[ishaderplat];
			if ((moreplatform != subplatform) &&
			    resource_platform_is_equal_or_more_specific(moreplatform, subplatform)) {
				// If we're looking at a super platform of requested platform, make sure shader
				// platform is also a super (not a different branch from the subplatform we're
				// iterating)
				if (superplatform && !resource_platform_is_equal_or_more_specific(platform, moreplatform))
					continue;
				for (imore = 0, moresize = array_size(moreplatforms); imore != moresize; ++imore) {
					if (moreplatforms[imore] == moreplatform)
						break;
				}
				if (imore == moresize)
					array_push(moreplatforms, shaderplatforms[ishaderplat]);
			}
		}
		array_deallocate(shaderplatforms);
	}

	if (superplatform)
		array_resize(subplatforms, 1);  // Clear out superplatforms

	for (imore = 0, moresize = array_size(moreplatforms); imore != moresize; ++imore) {
		for (iplat = 1, psize = array_size(subplatforms); (iplat != psize) && (result == 0); ++iplat) {
			if (moreplatforms[imore] == subplatforms[iplat])
				break;
		}
		if (iplat == psize)
			array_push(subplatforms, moreplatforms[imore]);
	}

	render_backend_t* prev_backend = render_backend_thread();

	for (iplat = 1, psize = array_size(subplatforms); iplat != psize; ++iplat) {
		stream_t* stream;
		uuid_t vertexshader, pixelshader;
		render_program_t* program = 0;
		uint64_t subplatform = subplatforms[iplat];
		if (subplatform == 0)
			continue;  // Programs are always platform specific

		platform_decl = resource_platform_decompose(subplatform);
		if (platform_decl.render_api <= RENDERAPI_DEFAULT) {
			if (platform_decl.render_api_group == RENDERAPIGROUP_OPENGL) {
				platform_decl.render_api = RENDERAPI_OPENGL;
			} else if (platform_decl.render_api_group == RENDERAPIGROUP_DIRECTX) {
				platform_decl.render_api = RENDERAPI_DIRECTX;
			} else if (platform_decl.render_api_group == RENDERAPIGROUP_GLES) {
				platform_decl.render_api = RENDERAPI_GLES;
			} else if (platform_decl.render_api_group == RENDERAPIGROUP_METAL) {
				platform_decl.render_api = RENDERAPI_METAL;
				if (prev_backend && (prev_backend->api == RENDERAPI_METAL))
					backend = prev_backend;
			} else {
				continue;  // Nonspecific render api
			}
		}

		if (!backend)
			backend = render_backend_allocate((render_api_t)platform_decl.render_api, true);
		if (!backend) {
			log_warn(HASH_RESOURCE, WARNING_UNSUPPORTED,
			         STRING_CONST("Unable to create render backend for shader compilation"));
			break;
		}

		render_drawable_t drawable;
		if (backend != prev_backend) {
#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
			window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render compile"), 100, 100,
			              WINDOW_FLAG_NOSHOW);
#else
			window_initialize(&window, nullptr);
#endif
			render_drawable_initialize_window(&drawable, &window, 0);

			render_backend_set_format(backend, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR);
			render_backend_set_drawable(backend, &drawable);
		}

		resource_change_t* shaderchange = resource_source_get(source, HASH_VERTEXSHADER, subplatform);
		vertexshader = string_to_uuid(STRING_ARGS(shaderchange->value.value));

		shaderchange = resource_source_get(source, HASH_PIXELSHADER, subplatform);
		pixelshader = string_to_uuid(STRING_ARGS(shaderchange->value.value));

		if ((platform_decl.render_api >= RENDERAPI_OPENGL) && (platform_decl.render_api <= RENDERAPI_OPENGL4))
			program = render_program_compile_opengl(backend, vertexshader, pixelshader);
		else if (platform_decl.render_api == RENDERAPI_METAL)
			program = render_program_compile_metal(backend, vertexshader, pixelshader);

		if (program) {
			stream = resource_local_create_static(uuid, subplatform);
			if (stream) {
				uint32_t version = RENDER_PROGRAM_RESOURCE_VERSION;
				resource_header_t header = {.type = resource_type_hash, .version = version, .source_hash = source_hash};
				size_t uuid_size = sizeof(uuid_t) * 2;
				size_t program_size = sizeof(render_program_t) + sizeof(render_parameter_t) * program->parameters_count;
				resource_stream_write_header(stream, header);
				stream_write_uint128(stream, vertexshader);
				stream_write_uint128(stream, pixelshader);
				stream_write(stream, pointer_offset(program, uuid_size), program_size - uuid_size);
				string_const_t streampath = stream_path(stream);
				log_infof(HASH_RESOURCE, STRING_CONST("Wrote compiled program static stream: %.*s"),
				          STRING_FORMAT(streampath));
				stream_deallocate(stream);

				result = 0;
			} else {
				log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL,
				           STRING_CONST("Unable to create static resource stream"));
			}

			render_program_deallocate(program);
		}

		if (backend != prev_backend) {
			render_backend_deallocate(backend);
			render_drawable_finalize(&drawable);
			window_finalize(&window);
		}
	}

	if (prev_backend)
		render_backend_enable_thread(prev_backend);

	array_deallocate(moreplatforms);
	array_deallocate(subplatforms);
	hashmap_finalize(map);

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
