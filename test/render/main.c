/* main.c  -  Render test  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
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
	app.company = string_const(STRING_CONST("Rampant Pixels"));
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

static void test_parse_config(const char* path, size_t path_size,
                              const char* buffer, size_t size,
                              const json_token_t* tokens, size_t num_tokens) {
	resource_module_parse_config(path, path_size, buffer, size, tokens, num_tokens);
	render_module_parse_config(path, path_size, buffer, size, tokens, num_tokens);
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
	window_t* window;

	EXPECT_TRUE(render_module_is_initialized());

	render_module_finalize();

	EXPECT_FALSE(render_module_is_initialized());

	memset(&config, 0, sizeof(render_config_t));
	render_module_initialize(config);

#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate(delegate_nswindow());
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate(delegate_uiwindow());
#elif FOUNDATION_PLATFORM_WINDOWS
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#elif FOUNDATION_PLATFORM_LINUX
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#else
#  error Not implemented
#endif

	EXPECT_NE(window, 0);
	EXPECT_TRUE(window_is_open(window));

	backend = render_backend_allocate(RENDERAPI_DEFAULT, true);

	EXPECT_NE(backend, 0);

	render_backend_deallocate(backend);

	window_deallocate(window);
	window = 0;

	EXPECT_FALSE(window_is_open(window));

	return 0;
}

static void*
_test_render_api(render_api_t api) {
	render_backend_t* backend = 0;
	render_resolution_t* resolutions = 0;
	render_drawable_t* drawable = 0;
	object_t framebuffer = 0;

	EXPECT_TRUE(render_module_is_initialized());

	window_t* window = 0;
#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate(delegate_nswindow());
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate(delegate_uiwindow());
#elif FOUNDATION_PLATFORM_WINDOWS
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#elif FOUNDATION_PLATFORM_LINUX
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#else
#  error Not implemented
#endif

	EXPECT_NE(window, 0);
	EXPECT_TRUE(window_is_open(window));

	backend = render_backend_allocate(api, false);

	if (!backend)
		goto ignore_test;

	EXPECT_EQ(render_backend_api(backend), api);

	resolutions = render_backend_enumerate_modes(backend, WINDOW_ADAPTER_DEFAULT);
	drawable = render_drawable_allocate();

	EXPECT_NE(backend, 0);
	EXPECT_NE(drawable, 0);

	log_infof(HASH_TEST, STRING_CONST("Resolution: %ux%u@%uHz"), resolutions[0].width,
	          resolutions[0].height, resolutions[0].refresh);

	render_drawable_set_window(drawable, window);

	log_infof(HASH_TEST, STRING_CONST("Drawable  : %ux%u"), render_drawable_width(drawable),
	          render_drawable_height(drawable));

	EXPECT_EQ(render_drawable_type(drawable), RENDERDRAWABLE_WINDOW);
	EXPECT_EQ(render_drawable_width(drawable), window_width(window));
	EXPECT_EQ(render_drawable_height(drawable), window_height(window));

	render_backend_set_format(backend, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR);
	render_backend_set_drawable(backend, drawable);

	framebuffer = render_backend_target_framebuffer(backend);
	EXPECT_NE(framebuffer, 0);
	EXPECT_GE(render_target_width(framebuffer), window_width(window));
	EXPECT_GE(render_target_height(framebuffer), window_height(window));
	EXPECT_EQ(render_target_pixelformat(framebuffer), PIXELFORMAT_R8G8B8X8);
	EXPECT_EQ(render_target_colorspace(framebuffer), COLORSPACE_LINEAR);

ignore_test:

	render_backend_deallocate(backend);
	render_drawable_deallocate(drawable);

	array_deallocate(resolutions);

	window_deallocate(window);
	window = 0;

	EXPECT_FALSE(window_is_open(window));

	return 0;
}


static void* _test_render_clear(render_api_t api) {
	render_backend_t* backend = 0;
	window_t* window = 0;
	render_drawable_t* drawable = 0;
	object_t framebuffer = 0;
	render_context_t* context = 0;

#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate(delegate_nswindow());
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate(delegate_uiwindow());
#elif FOUNDATION_PLATFORM_WINDOWS
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#elif FOUNDATION_PLATFORM_LINUX
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#else
#  error Not implemented
#endif

	backend = render_backend_allocate(api, false);

	if (!backend)
		goto ignore_test;

	//render_resolution_t* resolutions = render_backend_enumerate_modes( backend, WINDOW_ADAPTER_DEFAULT );
	drawable = render_drawable_allocate();

	render_drawable_set_window(drawable, window);

	render_backend_set_format(backend, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR);
	render_backend_set_drawable(backend, drawable);

	framebuffer = render_backend_target_framebuffer(backend);
	context = render_context_allocate(32);

	render_context_set_target(context, framebuffer);
	render_sort_reset(context);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        render_target_width(framebuffer), render_target_height(framebuffer), 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0x00000000, 0xF, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        render_target_width(framebuffer) / 2, render_target_height(framebuffer) / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0x1, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)),
	                        render_target_width(framebuffer) / 2, 0, render_target_width(framebuffer) / 2,
	                        render_target_height(framebuffer) / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0x2, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0,
	                        render_target_height(framebuffer) / 2, render_target_width(framebuffer) / 2,
	                        render_target_height(framebuffer) / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0x4, 1, 0);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)),
	                        render_target_width(framebuffer) / 2, render_target_height(framebuffer) / 2,
	                        render_target_width(framebuffer) / 2, render_target_height(framebuffer) / 2, 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0xFFFFFFFF, 0xF, 1, 0);

	render_sort_merge(&context, 1);
	render_backend_dispatch(backend, &context, 1);
	render_backend_flip(backend);

	//TODO: Verify framebuffer
	thread_sleep(2000);

ignore_test:

	render_context_deallocate(context);
	render_backend_deallocate(backend);
	render_drawable_deallocate(drawable);

	window_deallocate(window);
	window = 0;

	EXPECT_FALSE(window_is_open(window));

	return 0;
}

static void* _test_render_box(render_api_t api) {
	render_backend_t* backend = 0;
	window_t* window = 0;
	render_drawable_t* drawable = 0;
	object_t framebuffer = 0;
	object_t parameterbuffer = 0;
	object_t vertexbuffer = 0;
	object_t indexbuffer = 0;
	render_context_t* context = 0;
	object_t programobj = 0;
	render_program_t* program = nullptr;
	render_vertex_decl_t* vertex_decl = 0;
	matrix_t mvp;

	float32_t vertexdata[8 * 7] = {
		-0.5f,  0.5f,  0.5f,   1, 1, 1, 1,
		-0.5f, -0.5f,  0.5f,   0, 1, 0, 1,
		0.5f, -0.5f,  0.5f,   0, 0, 1, 1,
		0.5f,  0.5f,  0.5f,   1, 0, 0, 1,

		-0.5f,  0.5f, -0.5f,   1, 1, 0, 1,
		-0.5f, -0.5f, -0.5f,   1, 0, 1, 1,
		0.5f, -0.5f, -0.5f,   0, 1, 1, 1,
		0.5f,  0.5f, -0.5f,   0, 0, 0, 1
	};

	uint16_t indexdata[6 * 6] = {
		0, 1, 2,  0, 2, 3,
		1, 5, 6,  1, 6, 2,
		3, 2, 6,  3, 6, 7,
		7, 6, 5,  7, 5, 4,
		4, 3, 1,  4, 1, 0,
		4, 0, 3,  4, 3, 7
	};

#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate(delegate_nswindow());
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate(delegate_uiwindow());
#elif FOUNDATION_PLATFORM_WINDOWS
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#elif FOUNDATION_PLATFORM_LINUX
	window = window_create(WINDOW_ADAPTER_DEFAULT, STRING_CONST("Render test"), 800, 600, true);
#else
#  error Not implemented
#endif

	backend = (api != RENDERAPI_NULL) ? render_backend_allocate(api, false) : nullptr;

	if (!backend)
		goto ignore_test;

	//render_resolution_t* resolutions = render_backend_enumerate_modes( backend, WINDOW_ADAPTER_DEFAULT );
	drawable = render_drawable_allocate();

	render_drawable_set_window(drawable, window);

	render_backend_set_format(backend, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR);
	render_backend_set_drawable(backend, drawable);

	framebuffer = render_backend_target_framebuffer(backend);
	context = render_context_allocate(32);

	render_context_set_target(context, framebuffer);
	render_sort_reset(context);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        render_target_width(framebuffer), render_target_height(framebuffer), 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0x00000000, 0xFFFFFFFF, 1, 0);

	render_sort_merge(&context, 1);
	render_backend_dispatch(backend, &context, 1);
	render_backend_flip(backend);

	//color.program : 1ab9bba8-3f2f-4649-86bb-8b8b07e99af2
	uuid_t program_uuid = string_to_uuid(STRING_CONST("1ab9bba8-3f2f-4649-86bb-8b8b07e99af2"));
	programobj = render_program_load(backend, program_uuid);
	program = render_backend_program_resolve(backend, programobj);
	EXPECT_NE(programobj, 0);
	EXPECT_NE(program, nullptr);

	mvp = matrix_identity();

	parameterbuffer = render_parameterbuffer_create(backend, RENDERUSAGE_DYNAMIC,
	                                                program->parameters, program->num_parameters,
	                                                &mvp, sizeof(mvp));
	EXPECT_TYPENE(parameterbuffer, 0, object_t, PRIx64);

	render_parameterbuffer_link(parameterbuffer, program);

	vertex_decl = render_vertex_decl_allocate_varg(VERTEXFORMAT_FLOAT3, VERTEXATTRIBUTE_POSITION,
	                                               VERTEXFORMAT_FLOAT4, VERTEXATTRIBUTE_PRIMARYCOLOR,
	                                               VERTEXFORMAT_UNKNOWN);

	vertexbuffer = render_vertexbuffer_create(backend, RENDERUSAGE_STATIC, 8, vertex_decl, vertexdata);
	EXPECT_TYPENE(vertexbuffer, 0, object_t, PRIx64);

	indexbuffer = render_indexbuffer_create(backend, RENDERUSAGE_STATIC, 36, indexdata);
	EXPECT_TYPENE(indexbuffer, 0, object_t, PRIx64);

	render_sort_reset(context);

	render_command_viewport(render_context_reserve(context, render_sort_sequential_key(context)), 0, 0,
	                        render_target_width(framebuffer), render_target_height(framebuffer), 0, 1);
	render_command_clear(render_context_reserve(context, render_sort_sequential_key(context)),
	                     RENDERBUFFER_COLOR | RENDERBUFFER_DEPTH | RENDERBUFFER_STENCIL, 0x00000000, 0xFFFFFFFF, 1, 0);
	render_command_render(render_context_reserve(context, render_sort_sequential_key(context)),
	                      RENDERPRIMITIVE_TRIANGLELIST, 12,
	                      program, vertexbuffer, indexbuffer, parameterbuffer, 0);

	render_sort_merge(&context, 1);
	render_backend_dispatch(backend, &context, 1);
	render_backend_flip(backend);

	//TODO: Verify framebuffer

	thread_sleep(2000);

ignore_test:

	render_indexbuffer_destroy(indexbuffer);
	render_vertexbuffer_destroy(vertexbuffer);
	render_vertex_decl_deallocate(vertex_decl);
	render_parameterbuffer_destroy(parameterbuffer);
	if (backend)
		render_backend_program_release(backend, programobj);
	render_context_deallocate(context);
	render_backend_deallocate(backend);
	render_drawable_deallocate(drawable);

	window_deallocate(window);
	window = 0;

	EXPECT_FALSE(window_is_open(window));

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

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOSX || ( FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI )

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
#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOSX || FOUNDATION_PLATFORM_LINUX
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

static test_suite_t test_render_suite = {
	test_render_application,
	test_render_memory_system,
	test_render_config,
	test_render_declare,
	test_render_initialize,
	test_render_finalize,
	0
};


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


