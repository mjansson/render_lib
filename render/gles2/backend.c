/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#if FOUNDATION_PLATFORM_IOS

#  include <OpenGLES/ES2/gl.h>
#  include <OpenGLES/ES2/glext.h>

//Objective-C interface
RENDER_EXTERN const void*     _rb_gles2_create_egl_context( void );
RENDER_EXTERN void            _rb_gles2_destroy_egl_context( const void* context );
RENDER_EXTERN int             _rb_gles2_screen_width( void );
RENDER_EXTERN int             _rb_gles2_screen_height( void );

#endif


#if FOUNDATION_PLATFORM_IOS || FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI


typedef struct _render_backend_gles2
{
	RENDER_DECLARE_BACKEND;
    
	object_t                   render_thread;
    
#if FOUNDATION_PLATFORM_IOS
	const void*                context;
	GLuint                     framebuffer;
	GLuint                     renderbuffer;
	GLuint                     depthbuffer;
#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	EGLDisplay*                display;
	EGLConfig                  config;
	EGLSurface                 surface;
	EGLContext                 context;
#endif
    
	unsigned int               framebuffer_width;
	unsigned int               framebuffer_height;
    
} render_backend_gles2_t;


static bool _rb_gles2_construct( render_backend_t* backend )
{
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;
    
#if FOUNDATION_PLATFORM_IOS
    
	const void* context = _rb_gles2_create_egl_context();
	if( !context )
		return false;
    
	backend_gles2->context = context;
    
#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
    
	EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    
	int major = 2;
	int minor = 0;
	if( !eglInitialize( display, &major, &minor ) )
	{
		info_logf( "Unable to initialize EGL" );
		return false;
	}
	if( !eglBindAPI( EGL_OPENGL_ES_API ) )
	{
		info_logf( "Unable to bind OpenGL ES API" );
		return false;
	}
	debug_logf( "Initialized EGL v%d.%d", major, minor );
    
	backend_gles2->display = display;
    
#else
	FOUNDATION_ASSERT_FAIL( "GLES2 render backend platform not implemented" );
	return error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );
#endif
    
	backend_gles2->render_thread = thread_id();
    
    log_debug( HASH_RENDER, "Constructed GLES2 render backend" );
    return true;
}


static void _rb_gles2_destruct( render_backend_t* backend )
{
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;
    
#if FOUNDATION_PLATFORM_IOS
    
	_rb_gles2_destroy_egl_context( backend_gles2->context );
    
	backend_gles2->context = 0;
    
#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
    
	eglMakeCurrent( backend_gles2->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    
	eglTerminate( backend_gles2->display );
    
	backend_gles2->display = 0;
    
#  if FOUNDATION_PLATFORM_ANDROID
	if( backend_gles2->drawable->native )
		ANativeWindow_release( (ANativeWindow*)backend_gles2->drawable->native );
#  elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	//TODO: Implement
#  endif
    
#else
	FOUNDATION_ASSERT_FAIL( "GLES2 render backend platform not implemented" );
	error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );
	return;
#endif
    
    log_debug( HASH_RENDER, "Destructed GLES2 render backend" );
}


static unsigned int* _rb_gles2_enumerate_adapters( render_backend_t* backend )
{
    unsigned int* adapters = 0;
    array_push( adapters, WINDOW_ADAPTER_DEFAULT );
    return adapters;
}


static render_resolution_t* _rb_gles2_enumerate_modes( render_backend_t* backend, unsigned int adapter )
{
    render_resolution_t* modes = 0;
#if FOUNDATION_PLATFORM_IOS
    render_resolution_t mode = {
		0,
		_rb_gles2_screen_width(),
		_rb_gles2_screen_height(),
		PIXELFORMAT_R8G8B8X8,
		COLORSPACE_LINEAR,
		60
    };
#else
    render_resolution_t mode = { 0, 800, 600, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR, 60 };
#endif
    array_push( modes, mode );
    return modes;
}


static bool                    _rb_gles2_set_drawable( render_backend_t* backend, render_drawable_t* drawable ) { return true; }
static void                    _rb_gles2_dispatch( render_backend_t* backend, render_context_t** contexts, unsigned int num_contexts ) {}
static void                    _rb_gles2_flip( render_backend_t* backend ) { ++backend->framecount; }


static render_backend_vtable_t _render_backend_vtable_gles2 = {
	.construct = _rb_gles2_construct,
	.destruct  = _rb_gles2_destruct,
	.enumerate_adapters = _rb_gles2_enumerate_adapters,
	.enumerate_modes = _rb_gles2_enumerate_modes,
	.set_drawable = _rb_gles2_set_drawable,
	.dispatch = _rb_gles2_dispatch,
	.flip = _rb_gles2_flip
};


render_backend_t* render_backend_gles2_allocate()
{
	render_backend_t* backend = memory_allocate_zero_context( HASH_RENDER, sizeof( render_backend_t ), 0, MEMORY_PERSISTENT );
	backend->api = RENDERAPI_GLES2;
	backend->vtable = _render_backend_vtable_gles2;
	return backend;
}


#else


render_backend_t* render_backend_gles2_allocate()
{
	return 0;
}


#endif
