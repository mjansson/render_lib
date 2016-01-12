/* compile.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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
#include <window/window.h>

#include "gl4/glwrap.h"
#include "gl4/glprocs.h"

#if RESOURCE_ENABLE_LOCAL_SOURCE

int
render_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
               const char* type, size_t type_length) {
	if (render_shader_compile(uuid, platform, source, type, type_length) == 0)
		return 0;
	return -1;
}

static resource_change_t*
resource_source_platform_reduce(resource_change_t* change, resource_change_t* best, void* data) {
	uint64_t** subplatforms = data;
	uint64_t platform = (*subplatforms)[0];
	size_t iplat, psize;
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

int
render_shader_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
                      const char* type, size_t type_length) {
	int result = 0;
	resource_platform_t platform_decl;
	uint64_t* subplatforms = 0;
	size_t iplat, psize;
	hashmap_fixed_t fixedmap;
	hashmap_t* map = (hashmap_t*)&fixedmap;
	render_backend_t* backend = 0;
	window_t* window = 0;
	render_drawable_t* drawable = 0;
	//render_pixelshader_t pixelshader;
	//render_vertexshader_t vertexshader;

	if (!string_equal(type, type_length, STRING_CONST("vertexshader")) &&
	        !string_equal(type, type_length, STRING_CONST("pixelshader")))
		return -1;

	array_push(subplatforms, platform);

	hashmap_initialize(map, sizeof(fixedmap.bucket) / sizeof(fixedmap.bucket[0]), 8);
	resource_source_map_all(source, map, false);
	resource_source_map_reduce(source, map, &subplatforms, resource_source_platform_reduce);
	resource_source_map_clear(map);

	for (iplat = 1, psize = array_size(subplatforms); (iplat != psize) && (result == 0); ++iplat) {
		void* compiled_blob = 0;
		size_t compiled_size = 0;
		stream_t* stream;
		uint64_t subplatform = subplatforms[iplat];
		if (subplatform == 0)
			continue; //Shaders are always platform specific

		platform_decl = resource_platform_decompose(subplatform);
		if (platform_decl.render_api <= RENDERAPI_DEFAULT) {
			if (platform_decl.render_api_group == RENDERAPIGROUP_OPENGL)
				platform_decl.render_api = RENDERAPI_OPENGL;
			else if (platform_decl.render_api_group == RENDERAPIGROUP_DIRECTX)
				platform_decl.render_api = RENDERAPI_DIRECTX;
			else if (platform_decl.render_api_group == RENDERAPIGROUP_GLES)
				platform_decl.render_api = RENDERAPI_GLES;
			else
				continue; //Nonspecific render api
		}

		backend = render_backend_allocate(platform_decl.render_api, true);
		if (!backend) {
			log_warn(HASH_RESOURCE, WARNING_UNSUPPORTED,
			         STRING_CONST("Unable to create render backend for shader compilation"));
			result = -1;
			continue;
		}

#if FOUNDATION_PLATFORM_WINDOWS
		window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render compile"), 100, 100, false);
#endif

		drawable = render_drawable_allocate();
		render_drawable_set_window(drawable, window);

		render_backend_set_format(backend, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR);
		render_backend_set_drawable(backend, drawable);

		if ((platform_decl.render_api >= RENDERAPI_OPENGL) &&
		        (platform_decl.render_api <= RENDERAPI_OPENGL4)) {
			char* sourcebuffer = 0;
			resource_change_t* sourcechange = resource_source_get(source, HASH_SOURCE, subplatform);
			if (sourcechange && (sourcechange->flags & RESOURCE_SOURCEFLAG_BLOB)) {
				sourcebuffer = memory_allocate(HASH_RESOURCE, sourcechange->value.blob.size, 0, MEMORY_PERSISTENT);
				if (!resource_source_read_blob(uuid, HASH_SOURCE, subplatform, sourcechange->value.blob.checksum,
				                               sourcebuffer, sourcechange->value.blob.size)) {
					memory_deallocate(sourcebuffer);
					sourcebuffer = 0;
				}
			}
			if (sourcebuffer) {
				GLuint handle = glCreateShader(
				                    string_equal(type, type_length, STRING_CONST("vertexshader")) ?
				                    GL_VERTEX_SHADER_ARB : GL_FRAGMENT_SHADER_ARB);
				GLchar* glsource = (GLchar*)sourcebuffer;
				GLint source_size = (GLint)sourcechange->value.blob.size;
				glShaderSource(handle, 1, &glsource, &source_size);
				glCompileShader(handle);

				size_t log_capacity = 2048;
				GLchar* log_buffer = memory_allocate(HASH_RESOURCE, log_capacity, 0, MEMORY_TEMPORARY);
				GLint log_length = (GLint)log_capacity;
				GLint compiled = 0;
				glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);
				glGetShaderInfoLog(handle, log_length, &log_length, log_buffer);
				if (!compiled) {
					log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to compile shader: %.*s"),
					           (int)log_length, log_buffer);
					result = -1;
				}
				else {
					if (log_length < 2) {
						string_t nomsg = string_copy(log_buffer, log_capacity, STRING_CONST("<no message>"));
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
		}

		render_backend_deallocate(backend);
		render_drawable_deallocate(drawable);
		window_deallocate(window);

		stream = resource_local_create_static(uuid, subplatform);
		if (stream) {
			if (string_equal(type, type_length, STRING_CONST("vertexshader"))) {
				render_vertexshader_t shader;
				render_vertexshader_initialize(&shader);
				stream_write(stream, &shader, sizeof(shader));
			}
			else {
				render_pixelshader_t shader;
				render_pixelshader_initialize(&shader);
				stream_write(stream, &shader, sizeof(shader));
			}
			stream_deallocate(stream);

			if (compiled_size > 0) {
				stream = resource_local_create_dynamic(uuid, subplatform);
				if (stream) {
					stream_write(stream, compiled_blob, compiled_size);
					stream_deallocate(stream);
				}
				else {
					log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL,
					           STRING_CONST("Unable to create dynamic resource stream"));
					result = -1;
				}
			}
		}
		else {
			log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to create static resource stream"));
			result = -1;
		}

		memory_deallocate(compiled_blob);
	}

	return result;
}

#endif
