/* drawable.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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
#include <render/render.h>
#include <render/internal.h>

#if FOUNDATION_PLATFORM_WINDOWS
#  include <foundation/windows.h>
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#  include <EGL/egl.h>
#endif


render_drawable_t* render_drawable_allocate()
{
	return memory_allocate_zero_context( HASH_RENDER, sizeof( render_drawable_t ), 0, MEMORY_PERSISTENT );
}


void render_drawable_deallocate( render_drawable_t* drawable )
{
#if FOUNDATION_PLATFORM_WINDOWS
	if( drawable->hdc )
		window_release_hdc( drawable->hwnd, drawable->hdc );
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	if( drawable->native )
		memory_deallocate( drawable->native );
#endif
	memory_deallocate( drawable );
}


#if FOUNDATION_PLATFORM_IOS
void render_drawable_set_window( render_drawable_t* drawable, window_t* window, int tag )
#else
void render_drawable_set_window( render_drawable_t* drawable, window_t* window )
#endif
{
	drawable->type = RENDERDRAWABLE_WINDOW;
	drawable->adapter = window_adapter( window );
	drawable->window = window;
	drawable->width = window_width( window );
	drawable->height = window_height( window );
	drawable->refresh = 0;
#if FOUNDATION_PLATFORM_WINDOWS
	drawable->hwnd = window_hwnd( window );
	drawable->hdc = window_hdc( window );
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	//...
#elif FOUNDATION_PLATFORM_LINUX
	drawable->display = window_display( window );
	drawable->screen = window_screen( window );
	drawable->drawable = window_drawable( window );
#elif FOUNDATION_PLATFORM_MACOSX
	drawable->view = window_content_view( window );
#elif FOUNDATION_PLATFORM_IOS
	drawable->view = window_view( window, tag );
	drawable->drawable = window_layer( window, drawable->view );
	drawable->width = window_view_width( window, drawable->view );
	drawable->height = window_view_height( window, drawable->view );
#elif FOUNDATION_PLATFORM_ANDROID
	drawable->native = window_native( window );
	drawable->display = window_display( window );
#else
#  error Not implemented
#endif
}


void render_drawable_set_offscreen( render_drawable_t* drawable, object_t buffer )
{
	drawable->type = RENDERDRAWABLE_OFFSCREEN;
	drawable->buffer = buffer;
	FOUNDATION_ASSERT_FAIL( "Not implemented" );
}


void render_drawable_set_fullscreen( render_drawable_t* drawable, unsigned int adapter, unsigned int width, unsigned int height, unsigned int refresh )
{
	drawable->type = RENDERDRAWABLE_FULLSCREEN;
	drawable->adapter = adapter;
	drawable->width = width;
	drawable->height = height;
	drawable->refresh = refresh;
#if FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	drawable->display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
#endif
}


render_drawable_type_t render_drawable_type( render_drawable_t* drawable )
{
	return drawable->type;
}


unsigned int render_drawable_width( render_drawable_t* drawable )
{
	return drawable->width;
}


unsigned int render_drawable_height( render_drawable_t* drawable )
{
	return drawable->height;
}

