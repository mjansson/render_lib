/* drawable.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#if FOUNDATION_PLATFORM_WINDOWS
#  include <foundation/windows.h>
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#  include <EGL/egl.h>
#endif

render_drawable_t*
render_drawable_allocate(void) {
	return memory_allocate(HASH_RENDER, sizeof(render_drawable_t), 0,
	                       MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
}

void
render_drawable_deallocate(render_drawable_t* drawable) {
	render_drawable_finalize(drawable);
	memory_deallocate(drawable);
}

void
render_drawable_initialize_window(render_drawable_t* drawable, window_t* window, unsigned int tag) {
	drawable->type = RENDERDRAWABLE_WINDOW;
	drawable->adapter = window_adapter(window);
	drawable->window = window;
	drawable->tag = tag;
	drawable->width = (unsigned int)window_width(window);
	drawable->height = (unsigned int)window_height(window);
	drawable->refresh = 0;
#if FOUNDATION_PLATFORM_WINDOWS
	drawable->hwnd = window_hwnd(window);
	drawable->hdc = window_hdc(window);
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	//...
#elif FOUNDATION_PLATFORM_LINUX
	drawable->display = window_display(window);
	drawable->screen = window_screen(window);
	drawable->drawable = window_drawable(window);
#elif FOUNDATION_PLATFORM_MACOS
	drawable->view = window_view(window, tag);
#elif FOUNDATION_PLATFORM_IOS
	drawable->view = window_view(window, tag);
	drawable->drawable = window_layer(window, drawable->view);
	drawable->width = window_view_width(window, drawable->view);
	drawable->height = window_view_height(window, drawable->view);
#elif FOUNDATION_PLATFORM_ANDROID
	drawable->native = window_native(window);
	drawable->display = window_display(window);
#else
#  error Not implemented
#endif
}

void
render_drawable_initialize_fullscreen(render_drawable_t* drawable, unsigned int adapter,
                                      unsigned int width, unsigned int height, unsigned int refresh) {
	drawable->type = RENDERDRAWABLE_FULLSCREEN;
	drawable->adapter = adapter;
	drawable->width = width;
	drawable->height = height;
	drawable->refresh = refresh;
#if FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	drawable->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif
}

void
render_drawable_finalize(render_drawable_t* drawable) {
	if (drawable) {
#if FOUNDATION_PLATFORM_WINDOWS
		if (drawable->hdc)
			window_release_hdc(drawable->hwnd, drawable->hdc);
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
		if (drawable->native)
			memory_deallocate(drawable->native);
#endif
		memset(drawable, 0, sizeof(render_drawable_t));
	}
}
