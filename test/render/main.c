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

	EXPECT_TRUE(render_module_is_initialized());

	render_module_finalize();

	EXPECT_FALSE(render_module_is_initialized());

	memset(&config, 0, sizeof(render_config_t));
	render_module_initialize(config);

	backend = render_backend_allocate(RENDERAPI_DEFAULT, true);

	EXPECT_NE(backend, 0);

	render_backend_deallocate(backend);

	return 0;
}

static void*
_test_render_api(render_api_t api) {
	render_backend_t* backend = 0;
	render_resolution_t resolutions[32];
	//render_drawable_t* drawable = 0;
	//render_target_t* framebuffer = 0;

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
	log_infof(HASH_TEST, STRING_CONST("Resolution: %ux%u@%uHz"), resolutions[0].width, resolutions[0].height,
	          resolutions[0].refresh);


	render_target_t* render_target = render_target_window_allocate(backend, &window, 0);
	EXPECT_NE(render_target, nullptr);
	EXPECT_EQ(render_target->width, window_width(&window));
	EXPECT_EQ(render_target->height, window_height(&window));

	render_target_deallocate(render_target);

ignore_test:

	render_backend_deallocate(backend);

	window_finalize(&window);

	EXPECT_FALSE(window_is_open(&window));

	return 0;
}

static void*
_test_render_clear(render_api_t api) {
	render_backend_t* backend = 0;
	window_t window;
	render_target_t* target = 0;
	render_pipeline_t* pipeline = 0;

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

	target = render_target_window_allocate(backend, &window, 0);
	pipeline = render_pipeline_allocate(backend);

	render_pipeline_set_color_attachment(pipeline, 0, target);
	render_pipeline_set_color_clear(pipeline, 0, RENDERCLEAR_CLEAR, vector(1, 0, 0, 0));
	render_pipeline_set_depth_clear(pipeline, RENDERCLEAR_CLEAR, vector(0, 0, 0, 0));

	render_pipeline_flush(pipeline);

	// TODO: Verify framebuffer
	thread_sleep(5000);

	render_pipeline_deallocate(pipeline);
	render_target_deallocate(target);

ignore_test:

	render_backend_deallocate(backend);

	window_finalize(&window);

	return 0;
}

static void*
_test_render_box(render_api_t api) {
	render_backend_t* backend = 0;
	window_t window;
	render_target_t* target = 0;
	render_pipeline_t* pipeline = 0;

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

	target = render_target_window_allocate(backend, &window, 0);
	pipeline = render_pipeline_allocate(backend);

	render_pipeline_set_color_attachment(pipeline, 0, target);
	render_pipeline_set_color_clear(pipeline, 0, RENDERCLEAR_CLEAR, vector(1, 0, 0, 0));
	render_pipeline_set_depth_clear(pipeline, RENDERCLEAR_CLEAR, vector(0, 0, 0, 0));

	render_shader_t* shader = nullptr;
	if (api != RENDERAPI_NULL) {
		shader = render_shader_load(backend, uuid_decl(a7a465ed, 9fcb, 4383, 9494, abadc9b80eb9));
		EXPECT_NE(shader, 0);
	}

	const float32_t vertexdata[4 * 8] = { 0.5f,  0.5f,  0.0f, 1.0f, 1, 1, 1, 1,
	                                      0.5f, -0.5f,  0.0f, 1.0f, 0, 1, 0, 1,
	                                     -0.5f, -0.5f,  0.0f, 1.0f, 0, 0, 1, 1,
	                                     -0.5f,  0.5f,  0.0f, 1.0f, 1, 0, 0, 1};
	render_buffer_t* vertexbuffer = render_buffer_allocate(backend, RENDERUSAGE_STATIC, sizeof(vertexdata), vertexdata, sizeof(vertexdata));

	const uint16_t indexdata[2 * 3] = {0, 1, 2, 0, 2, 3};
	render_buffer_t* indexbuffer = render_buffer_allocate(backend, RENDERUSAGE_STATIC, sizeof(indexdata), indexdata, sizeof(indexdata));
	
	matrix_t mvp = matrix_identity();

	render_buffer_t* descriptor = render_buffer_allocate(backend, RENDERUSAGE_STATIC, sizeof(void*) + sizeof(matrix_t), 0, 0);
	// Vertex buffer
	render_buffer_argument_t argument[2] = {{
		.index = 0,
		.data_type = RENDERARGUMENT_POINTER,
		.size = 1
	},
	// Model-view-projection matrix
	{
		.index = 1,
		.data_type = RENDERARGUMENT_MATRIX4X4,
		.size = sizeof(matrix_t)
	}};
	render_buffer_argument_declare(descriptor, argument, 2);
	render_buffer_argument_encode_buffer(descriptor, 0, vertexbuffer, 0);
	render_buffer_argument_encode_constant(descriptor, 1, &mvp, sizeof(matrix_t));

	render_pipeline_set_color_attachment(pipeline, 0, target);
	render_pipeline_set_color_clear(pipeline, 0, RENDERCLEAR_CLEAR, vector(1, 0, 0, 0));
	render_pipeline_set_depth_clear(pipeline, RENDERCLEAR_CLEAR, vector(0, 0, 0, 0));

	render_pipeline_state_t pipeline_state;
	pipeline_state.shader = shader;

	render_primitive_t primitive;
	primitive.pipeline_state = &pipeline_state;
	primitive.index_buffer = indexbuffer;
	primitive.descriptor[0] = descriptor;

	render_pipeline_queue(pipeline, RENDERPRIMITIVE_TRIANGLELIST, &primitive);

	render_pipeline_flush(pipeline);

	thread_sleep(5000);

	render_buffer_deallocate(descriptor);
	render_buffer_deallocate(indexbuffer);
	render_buffer_deallocate(vertexbuffer);

	render_shader_unload(shader);

	render_pipeline_deallocate(pipeline);
	render_target_deallocate(target);

ignore_test:

	render_backend_deallocate(backend);

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

#if FOUNDATION_PLATFORM_APPLE

DECLARE_TEST(render, metal) {
	return _test_render_api(RENDERAPI_METAL);
}

DECLARE_TEST(render, metal_clear) {
	return _test_render_clear(RENDERAPI_METAL);
}

DECLARE_TEST(render, metal_box) {
	return _test_render_box(RENDERAPI_METAL);
}

#endif

static void
test_render_declare(void) {
	ADD_TEST(render, initialize);
	ADD_TEST(render, null);
	ADD_TEST(render, null_clear);
	ADD_TEST(render, null_box);
#if FOUNDATION_PLATFORM_APPLE
	ADD_TEST(render, metal);
	ADD_TEST(render, metal_clear);
	ADD_TEST(render, metal_box);
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
