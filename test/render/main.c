/* main.c  -  Render test  -  Public Domain  -  2013 Mattias Jansson
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/render_lib
 *
 * The foundation library source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <foundation/foundation.h>
#include <window/window.h>
#include <resource/resource.h>
#include <render/render.h>
#include <vector/vector.h>
#include <network/network.h>
#include <test/test.h>

static application_t
test_render_application(void) {
	application_t app;
	memset(&app, 0, sizeof(app));
	app.name = string_const(STRING_CONST("Render tests"));
	app.short_name = string_const(STRING_CONST("test_render"));
	app.company = string_const(STRING_CONST(""));
	app.version = render_module_version();
	app.exception_handler = test_exception_handler;
	return app;
}

static memory_system_t
test_render_memory_system(void) {
	return memory_system_malloc();
}

static foundation_config_t
test_render_config(void) {
	foundation_config_t config;
	memset(&config, 0, sizeof(config));
	return config;
}

static void
test_parse_config(const char* path, size_t path_size, const char* buffer, size_t size, const json_token_t* tokens,
                  size_t tokens_count) {
	resource_module_parse_config(path, path_size, buffer, size, tokens, tokens_count);
	render_module_parse_config(path, path_size, buffer, size, tokens, tokens_count);
}

static int
test_render_initialize(void) {
	window_config_t window_config;
	memset(&window_config, 0, sizeof(window_config));
	if (window_module_initialize(window_config))
		return -1;

	network_config_t network_config;
	memset(&network_config, 0, sizeof(network_config));
	if (network_module_initialize(network_config))
		return -1;

	resource_config_t resource_config;
	memset(&resource_config, 0, sizeof(resource_config));
	resource_config.enable_local_cache = true;
	resource_config.enable_local_source = true;
	resource_config.enable_local_autoimport = true;
	resource_config.enable_remote_sourced = true;
	resource_config.enable_remote_compiled = true;
	if (resource_module_initialize(resource_config))
		return -1;

	vector_config_t vector_config;
	memset(&vector_config, 0, sizeof(vector_config));
	if (vector_module_initialize(vector_config))
		return -1;

	render_config_t render_config;
	memset(&render_config, 0, sizeof(render_config));
	if (render_module_initialize(render_config))
		return -1;

	test_set_suitable_working_directory();
	test_load_config(test_parse_config);

	return 0;
}

static void
test_render_finalize(void) {
	render_module_finalize();
	vector_module_finalize();
	resource_module_finalize();
	network_module_finalize();
	window_module_finalize();
}

DECLARE_TEST(render, initialize) {
	render_config_t config;
	render_backend_t* backend;
	window_t window;

	EXPECT_TRUE(render_module_is_initialized());

	render_module_finalize();

	EXPECT_FALSE(render_module_is_initialized());

	memset(&config, 0, sizeof(render_config_t));
	render_module_initialize(config);

#if FOUNDATION_PLATFORM_MACOS || FOUNDATION_PLATFORM_IOS
	window_initialize(&window, delegate_window());
#elif FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
	window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, 0);
#else
#error Not implemented
#endif
	EXPECT_TRUE(window_is_open(&window));

	backend = render_backend_allocate(RENDERAPI_DEFAULT, true);

	EXPECT_NE(backend, 0);

	render_backend_deallocate(backend);

	window_finalize(&window);

	EXPECT_FALSE(window_is_open(&window));

	return 0;
}

static void*
_test_render_api(render_api_t api) {
	render_backend_t* backend = 0;
	render_resolution_t resolutions[32];
	render_drawable_t* drawable = 0;
	render_target_t* framebuffer = 0;

	EXPECT_TRUE(render_module_is_initialized());

	window_t window;
#if FOUNDATION_PLATFORM_MACOS || FOUNDATION_PLATFORM_IOS
	window_initialize(&window, delegate_window());
#elif FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
	window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, 0);
#else
#error Not implemented
#endif
	EXPECT_TRUE(window_is_open(&window));

	backend = render_backend_allocate(api, false);

	if (!backend)
		goto ignore_test;

	EXPECT_EQ(render_backend_api(backend), api);

	render_backend_enumerate_modes(backend, WINDOW_ADAPTER_DEFAULT, resolutions,
	                               sizeof(resolutions) / sizeof(resolutions[0]));
	drawable = render_drawable_allocate();

	EXPECT_NE(backend, 0);
	EXPECT_NE(drawable, 0);

	log_infof(HASH_TEST, STRING_CONST("Resolution: %ux%u@%uHz"), resolutions[0].width, resolutions[0].height,
	          resolutions[0].refresh);

	render_drawable_initialize_window(drawable, &window, 0);

	log_infof(HASH_TEST, STRING_CONST("Drawable  : %ux%u"), drawable->width, drawable->height);

	EXPECT_EQ(drawable->type, RENDERDRAWABLE_WINDOW);
	EXPECT_EQ(drawable->width, window_width(&window));
	EXPECT_EQ(drawable->height, window_height(&window));

	render_backend_set_format(backend, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR);
	render_backend_set_drawable(backend, drawable);

	framebuffer = render_backend_target_framebuffer(backend);
	EXPECT_NE(framebuffer, 0);
	EXPECT_GE(framebuffer->width, window_width(&window));
	EXPECT_GE(framebuffer->height, window_height(&window));
	EXPECT_EQ(framebuffer->pixelformat, PIXELFORMAT_R8G8B8);
	EXPECT_EQ(framebuffer->colorspace, COLORSPACE_LINEAR);

ignore_test:

	render_backend_deallocate(backend);
	render_drawable_deallocate(drawable);

	window_finalize(&window);

	EXPECT_FALSE(window_is_open(&window));

	return 0;
}

static void*
_test_render_clear(render_api_t api) {
	render_backend_t* backend = 0;
	window_t window;
	render_drawable_t* drawable = 0;
	render_target_t* framebuffer = 0;
	render_context_t* context = 0;

#if FOUNDATION_PLATFORM_MACOS || FOUNDATION_PLATFORM_IOS
	window_initialize(&window, delegate_window());
#elif FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
	window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, 0);
#else
#error Not implemented
#endif

	backend = render_backend_allocate(api, false);

	if (!backend)
		goto ignore_test;

	drawable = render_drawable_allocate();

	render_drawable_initialize_window(drawable, &window, 0);

	render_backend_set_format(backend, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR);
	render_backend_set_drawable(backend, drawable);

	framebuffer = render_backend_target_framebuffer(backend);
	context = render_context_allocate(32);

	render_sort_reset(context);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        framebuffer->width, framebuffer->height, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0x00000000, 0xF, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        framebuffer->width / 2, framebuffer->height / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0x1, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)),
	                        framebuffer->width / 2, 0, framebuffer->height / 2, framebuffer->height / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0x2, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0,
	                        framebuffer->height / 2, framebuffer->width / 2, framebuffer->height / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0x4, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)),
	                        framebuffer->width / 2, framebuffer->height / 2, framebuffer->width / 2,
	                        framebuffer->height / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0xF, 1, 0);

	render_sort_merge(&context, 1);
	render_backend_dispatch(backend, framebuffer, &context, 1);
	render_backend_flip(backend);

	// TODO: Verify framebuffer
	thread_sleep(2000);

ignore_test:

	render_context_deallocate(context);
	render_backend_deallocate(backend);
	render_drawable_deallocate(drawable);

	window_finalize(&window);

	return 0;
}

static void*
_test_render_box(render_api_t api) {
	render_backend_t* backend = 0;
	window_t window;
	render_drawable_t* drawable = 0;
	render_target_t* framebuffer = 0;
	render_parameterbuffer_t* parameterbuffer = 0;
	render_vertexbuffer_t* vertexbuffer = 0;
	render_indexbuffer_t* indexbuffer = 0;
	render_context_t* context = 0;
	render_program_t* program = nullptr;
	render_vertex_decl_t* vertex_decl = 0;
	matrix_t mvp;

	float32_t vertexdata[8 * 7] = {-0.5f, 0.5f,  0.5f,  1, 1, 1, 1, -0.5f, -0.5f, 0.5f,  0, 1, 0, 1,
	                               0.5f,  -0.5f, 0.5f,  0, 0, 1, 1, 0.5f,  0.5f,  0.5f,  1, 0, 0, 1,

	                               -0.5f, 0.5f,  -0.5f, 1, 1, 0, 1, -0.5f, -0.5f, -0.5f, 1, 0, 1, 1,
	                               0.5f,  -0.5f, -0.5f, 0, 1, 1, 1, 0.5f,  0.5f,  -0.5f, 0, 0, 0, 1};

	uint16_t indexdata[6 * 6] = {0, 1, 2, 0, 2, 3, 1, 5, 6, 1, 6, 2, 3, 2, 6, 3, 6, 7,
	                             7, 6, 5, 7, 5, 4, 4, 3, 1, 4, 1, 0, 4, 0, 3, 4, 3, 7};

#if FOUNDATION_PLATFORM_MACOS || FOUNDATION_PLATFORM_IOS
	window_initialize(&window, delegate_window());
#elif FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_LINUX
	window_create(&window, WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, 0);
#else
#error Not implemented
#endif

	backend = (api != RENDERAPI_NULL) ? render_backend_allocate(api, false) : nullptr;

	if (!backend)
		goto ignore_test;

	drawable = render_drawable_allocate();

	render_drawable_initialize_window(drawable, &window, 0);

	render_backend_set_format(backend, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR);
	render_backend_set_drawable(backend, drawable);

	framebuffer = render_backend_target_framebuffer(backend);
	context = render_context_allocate(32);

	render_sort_reset(context);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        framebuffer->width, framebuffer->height, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0x00000000, 0xFFFFFFFF, 1, 0);

	render_sort_merge(&context, 1);
	render_backend_dispatch(backend, framebuffer, &context, 1);
	render_backend_flip(backend);

	// color.program : 1ab9bba8-3f2f-4649-86bb-8b8b07e99af2
	uuid_t program_uuid = uuid_make(0x46493f2f1ab9bba8, 0xf29ae9078b8bbb86);
	program = render_program_load(backend, program_uuid);
	EXPECT_NE(program, nullptr);

	mvp = matrix_identity();

	parameterbuffer = render_parameterbuffer_allocate(backend, RENDERUSAGE_DYNAMIC, program->parameters,
	                                                  program->parameters_count, &mvp, sizeof(mvp));
	EXPECT_NE(parameterbuffer, 0);

	render_parameterbuffer_link(parameterbuffer, program);

	vertex_decl = render_vertex_decl_allocate_varg(VERTEXFORMAT_FLOAT3, VERTEXATTRIBUTE_POSITION, VERTEXFORMAT_FLOAT4,
	                                               VERTEXATTRIBUTE_PRIMARYCOLOR, VERTEXFORMAT_UNUSED);

	vertexbuffer = render_vertexbuffer_allocate(backend, RENDERUSAGE_STATIC, 8, sizeof(vertexdata), vertex_decl,
	                                            vertexdata, sizeof(vertexdata));
	EXPECT_NE(vertexbuffer, 0);

	indexbuffer = render_indexbuffer_allocate(backend, RENDERUSAGE_STATIC, 36, sizeof(indexdata), INDEXFORMAT_USHORT,
	                                          indexdata, sizeof(indexdata));
	EXPECT_NE(indexbuffer, 0);

	render_sort_reset(context);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        framebuffer->width, framebuffer->height, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0x00000000, 0xFFFFFFFF, 1, 0);
	render_command_render(render_context_reserve(context, render_sort_sequential_key(context)),
	                      RENDERPRIMITIVE_TRIANGLELIST, 12, program, vertexbuffer, indexbuffer, parameterbuffer, 0);

	render_sort_merge(&context, 1);
	render_backend_dispatch(backend, framebuffer, &context, 1);
	render_backend_flip(backend);

	// TODO: Verify framebuffer

	thread_sleep(2000);

ignore_test:

	render_indexbuffer_deallocate(indexbuffer);
	render_vertexbuffer_deallocate(vertexbuffer);
	render_vertex_decl_deallocate(vertex_decl);
	render_parameterbuffer_deallocate(parameterbuffer);
	render_program_unload(program);
	render_context_deallocate(context);
	render_backend_deallocate(backend);
	render_drawable_deallocate(drawable);

	window_finalize(&window);

	return 0;
}

DECLARE_TEST(render, null) {
	return _test_render_api(RENDERAPI_NULL);
}

DECLARE_TEST(render, null_clear) {
	return _test_render_clear(RENDERAPI_NULL);
}

DECLARE_TEST(render, null_box) {
	return _test_render_box(RENDERAPI_NULL);
}

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOS || \
    (FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI)

DECLARE_TEST(render, gl4) {
	return _test_render_api(RENDERAPI_OPENGL4);
}

DECLARE_TEST(render, gl4_clear) {
	return _test_render_clear(RENDERAPI_OPENGL4);
}

DECLARE_TEST(render, gl4_box) {
	return _test_render_box(RENDERAPI_OPENGL4);
}

DECLARE_TEST(render, gl2) {
	return _test_render_api(RENDERAPI_OPENGL2);
}

DECLARE_TEST(render, gl2_clear) {
	return _test_render_clear(RENDERAPI_OPENGL2);
}

DECLARE_TEST(render, gl2_box) {
	return _test_render_box(RENDERAPI_OPENGL2);
}

#endif

#if FOUNDATION_PLATFORM_IOS || FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

DECLARE_TEST(render, gles2) {
	return _test_render_api(RENDERAPI_GLES2);
}

DECLARE_TEST(render, gles2_clear) {
	return _test_render_clear(RENDERAPI_GLES2);
}

DECLARE_TEST(render, gles2_box) {
	return _test_render_box(RENDERAPI_GLES2);
}

#endif

static void
test_render_declare(void) {
	ADD_TEST(render, initialize);
	ADD_TEST(render, null);
	ADD_TEST(render, null_clear);
	ADD_TEST(render, null_box);
#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOS || FOUNDATION_PLATFORM_LINUX
	ADD_TEST(render, gl4);
	ADD_TEST(render, gl4_clear);
	ADD_TEST(render, gl4_box);
	ADD_TEST(render, gl2);
	ADD_TEST(render, gl2_clear);
	ADD_TEST(render, gl2_box);
#endif
#if FOUNDATION_PLATFORM_IOS || FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	ADD_TEST(render, gles2);
	ADD_TEST(render, gles2_clear);
	ADD_TEST(render, gles2_box);
#endif
}

static test_suite_t test_render_suite = {test_render_application,
                                         test_render_memory_system,
                                         test_render_config,
                                         test_render_declare,
                                         test_render_initialize,
                                         test_render_finalize,
                                         0};

#if BUILD_MONOLITHIC

int
test_render_run(void);

int
test_render_run(void) {
	test_suite = test_render_suite;
	return test_run_all();
}

#else

test_suite_t
test_suite_define(void);

test_suite_t
test_suite_define(void) {
	return test_render_suite;
}

#endif
