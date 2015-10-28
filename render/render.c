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
#include <window/window.h>
#include <render/render.h>
#include <render/internal.h>

static bool _render_initialized = false;

int
render_module_initialize(render_config_t config) {
	if (_render_initialized)
		return 0;

	FOUNDATION_UNUSED(config);

	_render_api_disabled[RENDERAPI_UNKNOWN] = true;
	_render_api_disabled[RENDERAPI_DEFAULT] = true;

	if (render_target_initialize() < 0)
		return -1;

	_render_initialized = true;

	return 0;
}

void
render_module_finalize(void) {
	if (!_render_initialized)
		return;

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
		if ((api[i] > RENDERAPI_DEFAULT) && (api[i] < RENDERAPI_NUM))
			_render_api_disabled[api[i]] = false;
	}
}

void
render_api_disable(const render_api_t* api, size_t num) {
	for (size_t i = 0; i < num; ++i) {
		if ((api[i] > RENDERAPI_DEFAULT) && (api[i] < RENDERAPI_NUM))
			_render_api_disabled[api[i]] = true;
	}
}
