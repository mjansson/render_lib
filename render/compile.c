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

#if RESOURCE_ENABLE_LOCAL_SOURCE

int
render_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
               const char* type, size_t type_length) {
	if (render_shader_compile(uuid, platform, source, type, type_length) == 0)
		return 0;
	return -1;
}

int
render_shader_compile(const uuid_t uuid, uint64_t platform, resource_source_t* source,
                      const char* type, size_t type_length) {
	int result = -1;

	FOUNDATION_UNUSED(platform);

	if (string_equal(type, type_length, STRING_CONST("vertexshader"))) {
		log_info(HASH_RESOURCE, STRING_CONST("Compiling vertex shader"));
		result = 0;
	}
	else if (string_equal(type, type_length, STRING_CONST("pixelshader"))) {
		log_info(HASH_RESOURCE, STRING_CONST("Compiling pixel shader"));
		result = 0;
	}

	return result;
}

#endif
