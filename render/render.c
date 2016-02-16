/* render.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

#include <resource/compile.h>

#include <render/render.h>
#include <render/internal.h>

static bool _render_initialized;

//Global data
render_config_t _render_config;
bool _render_api_disabled[RENDERAPI_NUM];
objectmap_t* _render_map_target;
objectmap_t* _render_map_buffer;

int
render_module_initialize(render_config_t config) {
	if (_render_initialized)
		return 0;

	_render_config.target_max = config.target_max    ?
	                            config.target_max    : 32;

	_render_config.buffer_max = config.buffer_max    ?
	                            config.buffer_max    : 1024;

	_render_api_disabled[RENDERAPI_UNKNOWN] = true;
	_render_api_disabled[RENDERAPI_DEFAULT] = true;
	_render_api_disabled[RENDERAPI_OPENGL] = true;
	_render_api_disabled[RENDERAPI_DIRECTX] = true;
	_render_api_disabled[RENDERAPI_GLES] = true;

	if (render_target_initialize() < 0)
		return -1;

	if (render_buffer_initialize() < 0)
		return -1;

#if RESOURCE_ENABLE_LOCAL_CACHE && RESOURCE_ENABLE_LOCAL_SOURCE
	resource_compile_register(render_compile);
#endif

	_render_initialized = true;

	return 0;
}

void
render_module_finalize(void) {
	if (!_render_initialized)
		return;

	render_buffer_finalize();
	render_target_finalize();

	_render_initialized = false;
}

bool
render_module_is_initialized(void) {
	return _render_initialized;
}

void
render_api_enable(const render_api_t* api, size_t num) {
	for (size_t i = 0; i < num; ++i) {
		if ((api[i] > RENDERAPI_DEFAULT) && (api[i] < RENDERAPI_NUM)) {
			if ((api[i] != RENDERAPI_OPENGL) &&
			        (api[i] != RENDERAPI_DIRECTX) &&
			        (api[i] != RENDERAPI_GLES))
				_render_api_disabled[api[i]] = false;
		}
	}
}

void
render_api_disable(const render_api_t* api, size_t num) {
	for (size_t i = 0; i < num; ++i) {
		if ((api[i] > RENDERAPI_DEFAULT) && (api[i] < RENDERAPI_NUM))
			_render_api_disabled[api[i]] = true;
	}
}
