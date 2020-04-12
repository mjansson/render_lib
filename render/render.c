/* render.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#include <resource/import.h>
#include <resource/compile.h>

#include <render/render.h>
#include <render/compile.h>
#include <render/import.h>
#include <render/internal.h>

static bool _render_initialized;

// Global data
render_config_t _render_config;
bool _render_api_disabled[RENDERAPI_COUNT];
render_backend_t** _render_backends;

int
render_module_initialize(render_config_t config) {
	if (_render_initialized)
		return 0;

	_render_config.target_max = config.target_max ? config.target_max : 32;

	_render_config.buffer_max = config.buffer_max ? config.buffer_max : 1024;

	_render_config.program_max = config.program_max ? config.program_max : 128;

	_render_api_disabled[RENDERAPI_UNKNOWN] = true;
	_render_api_disabled[RENDERAPI_DEFAULT] = true;
	_render_api_disabled[RENDERAPI_OPENGL] = true;
	_render_api_disabled[RENDERAPI_DIRECTX] = true;
	_render_api_disabled[RENDERAPI_GLES] = true;

	resource_import_register(render_import);
	resource_compile_register(render_compile);

	_render_initialized = true;

	return 0;
}

void
render_module_finalize(void) {
	if (!_render_initialized)
		return;

	array_deallocate(_render_backends);

	_render_initialized = false;
}

bool
render_module_is_initialized(void) {
	return _render_initialized;
}

void
render_api_enable(const render_api_t* api, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if ((api[i] > RENDERAPI_DEFAULT) && (api[i] < RENDERAPI_COUNT)) {
			if ((api[i] != RENDERAPI_OPENGL) && (api[i] != RENDERAPI_DIRECTX) && (api[i] != RENDERAPI_GLES))
				_render_api_disabled[api[i]] = false;
		}
	}
}

void
render_api_disable(const render_api_t* api, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if ((api[i] > RENDERAPI_DEFAULT) && (api[i] < RENDERAPI_COUNT))
			_render_api_disabled[api[i]] = true;
	}
}

void
render_module_parse_config(const char* path, size_t path_size, const char* buffer, size_t size,
                           const json_token_t* tokens, size_t tokens_count) {
	FOUNDATION_UNUSED(path);
	FOUNDATION_UNUSED(path_size);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	FOUNDATION_UNUSED(tokens);
	FOUNDATION_UNUSED(tokens_count);
}
