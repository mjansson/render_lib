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
#include <foundation/delegate.h>
#include <window/window.h>
#include <render/render.h>
#include <test/test.h>


static application_t test_render_application( void )
{
	application_t app = {0};
	app.name = "Render tests";
	app.short_name = "test_render";
	app.config_dir = "test_render";
	return app;
}


static memory_system_t test_render_memory_system( void )
{
	return memory_system_malloc();
}


static int test_render_initialize( void )
{
	return 0;
}


static void test_render_shutdown( void )
{
}


DECLARE_TEST( render, initialize )
{
    render_initialize();
    
    EXPECT_TRUE( render_is_initialized() );
    
    render_shutdown();
    
    EXPECT_FALSE( render_is_initialized() );
    
	window_t* window;
#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate_from_nswindow( delegate_nswindow() );
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate_from_uiwindow( delegate_uiwindow() );
#else
#  error Not implemented
#endif
	
	EXPECT_NE( window, 0 );
	EXPECT_TRUE( window_is_open( window ) );
	
    render_backend_t* backend = render_backend_allocate( RENDERAPI_DEFAULT );
    
    
    render_backend_deallocate( backend );
    
	window_deallocate( window );
	window = 0;
	
	EXPECT_FALSE( window_is_open( window ) );
    
	return 0;
}


DECLARE_TEST( render, null )
{
    render_initialize();
    
    EXPECT_TRUE( render_is_initialized() );
    
    render_shutdown();
    
    EXPECT_FALSE( render_is_initialized() );
    
	window_t* window;
#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate_from_nswindow( delegate_nswindow() );
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate_from_uiwindow( delegate_uiwindow() );
#else
#  error Not implemented
#endif
	
	EXPECT_NE( window, 0 );
	EXPECT_TRUE( window_is_open( window ) );
    
    render_backend_t* backend = render_backend_allocate( RENDERAPI_NULL );

    EXPECT_EQ( render_backend_api( backend ), RENDERAPI_NULL );
    
    render_drawable_t* drawable = render_drawable_allocate();
    
    EXPECT_NE( backend, 0 );
    EXPECT_NE( drawable, 0 );
    
#if FOUNDATION_PLATFORM_IOS
    render_drawable_set_window( drawable, window, 1 );
#else
    render_drawable_set_window( drawable, window );
#endif
    
    EXPECT_EQ( render_drawable_type( drawable ), RENDERDRAWABLE_WINDOW );
	EXPECT_EQ( render_drawable_width( drawable ), window_width( window ) );
	EXPECT_EQ( render_drawable_height( drawable ), window_height( window ) );
    
    render_backend_set_format( backend, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR );
    render_backend_set_drawable( backend, drawable );
    
    render_backend_deallocate( backend );
    
	window_deallocate( window );
	window = 0;
    
	EXPECT_FALSE( window_is_open( window ) );
    
	return 0;
}


DECLARE_TEST( render, gles2 )
{
    render_initialize();
    
    EXPECT_TRUE( render_is_initialized() );
    
    render_shutdown();
    
    EXPECT_FALSE( render_is_initialized() );
    
	window_t* window;
#if FOUNDATION_PLATFORM_MACOSX
	window = window_allocate_from_nswindow( delegate_nswindow() );
#elif FOUNDATION_PLATFORM_IOS
	window = window_allocate_from_uiwindow( delegate_uiwindow() );
#else
#  error Not implemented
#endif
	
	EXPECT_NE( window, 0 );
	EXPECT_TRUE( window_is_open( window ) );
    
    render_backend_t* backend = render_backend_allocate( RENDERAPI_GLES2 );
    
    EXPECT_EQ( render_backend_api( backend ), RENDERAPI_GLES2 );
	
	render_resolution_t* resolutions = render_backend_enumerate_modes( backend, WINDOW_ADAPTER_DEFAULT );
    render_drawable_t* drawable = render_drawable_allocate();
    
    EXPECT_NE( backend, 0 );
    EXPECT_NE( drawable, 0 );

	log_infof( HASH_TEST, "Render mode: %ux%u@%uHz", resolutions[0].width, resolutions[0].height, resolutions[0].refresh );
    
#if FOUNDATION_PLATFORM_IOS
    render_drawable_set_window( drawable, window, 1 );
#else
    render_drawable_set_window( drawable, window );
#endif

	log_infof( HASH_TEST, "Drawable   : %ux%u", render_drawable_width( drawable ), render_drawable_height( drawable ) );
    
    EXPECT_EQ( render_drawable_type( drawable ), RENDERDRAWABLE_WINDOW );
	EXPECT_EQ( render_drawable_width( drawable ), window_width( window ) );
	EXPECT_EQ( render_drawable_height( drawable ), window_height( window ) );
    
    render_backend_set_format( backend, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR );
    render_backend_set_drawable( backend, drawable );
    
	thread_sleep( 5000 );
	
    render_backend_deallocate( backend );
    
	window_deallocate( window );
	window = 0;
    
	EXPECT_FALSE( window_is_open( window ) );
    
	return 0;
}


static void test_render_declare( void )
{
	ADD_TEST( render, initialize );
	ADD_TEST( render, null );
#if FOUNDATION_PLATFORM_IOS || FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	ADD_TEST( render, gles2 );
#endif
}


test_suite_t test_render_suite = {
	test_render_application,
	test_render_memory_system,
	test_render_declare,
	test_render_initialize,
	test_render_shutdown
};


#if FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_IOS

int test_render_run( void );
int test_render_run( void )
{
	test_suite = test_render_suite;
	return test_run_all();
}

#else

test_suite_t test_suite_define( void );
test_suite_t test_suite_define( void )
{
	return test_render_suite;
}

#endif


