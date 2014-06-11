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
RENDER_EXTERN const void*     _rb_gles2_ios_create_egl_context( void );
RENDER_EXTERN void            _rb_gles2_ios_set_current_egl_context( const void* context );
RENDER_EXTERN void            _rb_gles2_ios_destroy_egl_context( const void* context );
RENDER_EXTERN int             _rb_gles2_ios_screen_width( void );
RENDER_EXTERN int             _rb_gles2_ios_screen_height( void );
RENDER_EXTERN bool            _rb_gles2_ios_render_buffer_storage_from_drawable( const void* context, const void* drawable, unsigned int framebuffer, unsigned int colorbuffer );

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


#if FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

const char* _rb_gles2_egl_error_message( EGLint err )
{
	switch( err )
	{
		case EGL_NOT_INITIALIZED:
			return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS:
			return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC:
			return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE:
			return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONFIG:
			return "EGL_BAD_CONFIG";
		case EGL_BAD_CONTEXT:
			return "EGL_BAD_CONTEXT";
		case EGL_BAD_CURRENT_SURFACE:
			return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY:
			return "EGL_BAD_DISPLAY";
		case EGL_BAD_MATCH:
			return "EGL_BAD_MATCH";
		case EGL_BAD_NATIVE_PIXMAP:
			return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW:
			return "EGL_BAD_NATIVE_WINDOW";
		case EGL_BAD_PARAMETER:
			return "EGL_BAD_PARAMETER";
		case EGL_BAD_SURFACE:
			return "EGL_BAD_SURFACE";
		case EGL_CONTEXT_LOST:
			return "EGL_CONTEXT_LOST";
		default:
			break;
	}
	return "<UNKNOWN>";
}

#endif


const char* _rb_gles2_error_message( GLenum err )
{
	switch( err )
	{
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		default:
			break;
	}
	return "<UNKNOWN>";
}


bool _rb_gles2_check_error( const char* message )
{
	GLenum err = glGetError();
	if( err != GL_NONE )
	{
		log_errorf( HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, "%s: %s", message, _rb_gles2_error_message( err ) );
		FOUNDATION_ASSERT_FAILFORMAT( "OpenGLES2 error: %s: %s", message, _rb_gles2_error_message( err ) );
		return true;
	}
	return false;
}


static bool _rb_gles2_construct( render_backend_t* backend )
{
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;
    
#if FOUNDATION_PLATFORM_IOS
    
	const void* context = _rb_gles2_ios_create_egl_context();
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
    
	_rb_gles2_ios_destroy_egl_context( backend_gles2->context );
    
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
		_rb_gles2_ios_screen_width(),
		_rb_gles2_ios_screen_height(),
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


static void _rb_gles2_enable_thread( render_backend_t* backend )
{
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;
#if FOUNDATION_PLATFORM_IOS
	_rb_gles2_ios_set_current_egl_context( backend_gles2->context );
#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	if( !eglMakeCurrent( backend_gles2->display, backend_gles2->surface, backend_gles2->surface, backend_gles2->context ) )
		log_errorf( HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, "Unable to make context current on thread: %s", eglGetErrorMessage( eglGetError() ) );
#else
	FOUNDATION_ASSERT_FAIL( "GLES2 render backend platform not implemented" );
	error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );
#endif
}


static void _rb_gles2_disable_thread( render_backend_t* backend )
{
}


static bool _rb_gles2_set_drawable( render_backend_t* backend, render_drawable_t* drawable )
{
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;
	
	if( drawable->type == RENDERDRAWABLE_OFFSCREEN )
	{
		FOUNDATION_ASSERT_FAIL( "GLES2 offscreen drawable not implemented" );
		return error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );
	}
	
#if FOUNDATION_PLATFORM_IOS
	
	glGenFramebuffers( 1, &backend_gles2->framebuffer );
	glBindFramebuffer( GL_FRAMEBUFFER, backend_gles2->framebuffer );
	_rb_gles2_check_error( "Unable to generate framebuffer object" );
	
	glGenRenderbuffers( 1, &backend_gles2->renderbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, backend_gles2->renderbuffer );
	_rb_gles2_check_error( "Unable to generate renderbuffer object" );
	
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, backend_gles2->renderbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, backend_gles2->renderbuffer );
	_rb_gles2_ios_render_buffer_storage_from_drawable( backend_gles2->context, drawable->drawable, backend_gles2->framebuffer, backend_gles2->renderbuffer );
	
	GLint width = 0, height = 0;
	glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width );
	glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height );
	
	glGenRenderbuffers( 1, &backend_gles2->depthbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, backend_gles2->depthbuffer );
	_rb_gles2_check_error( "Unable to generate depthbuffer object" );
	
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, backend_gles2->depthbuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height );
	_rb_gles2_check_error( "Unable to allocate storage for depthbuffer object" );
	
	glBindRenderbuffer( GL_RENDERBUFFER, backend_gles2->renderbuffer ); //restore color binding
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		log_errorf( HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, "Failed to make complete framebuffer object with dimensions %dx%d: %d", width, height, glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
		return false;
	}
	
	log_infof( HASH_RENDER, "Initialized complete framebuffer object with dimensions %dx%d", width, height );
	
	backend_gles2->framebuffer_width = width;
	backend_gles2->framebuffer_height = height;

#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	
	FOUNDATION_ASSERT_MSG( drawable->display == backend_gles2->display, "Invalid drawable display" );
	eglMakeCurrent( drawable->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
	
#define MAX_CONFIG_ATTRS_SIZE 32
#define MAX_MATCHING_CONFIGS  64
	
	EGLint config_attrs[MAX_CONFIG_ATTRS_SIZE];
	EGLint i = 0;
	int ret = 0;
	
	// construct attribute request
	config_attrs[i]   = EGL_SURFACE_TYPE;
	config_attrs[++i] = EGL_WINDOW_BIT;
	config_attrs[++i] = EGL_RENDERABLE_TYPE;
	config_attrs[++i] = EGL_OPENGL_ES2_BIT;
	config_attrs[++i] = EGL_RED_SIZE;
	config_attrs[++i] = 8;
	config_attrs[++i] = EGL_GREEN_SIZE;
	config_attrs[++i] = 8;
	config_attrs[++i] = EGL_BLUE_SIZE;
	config_attrs[++i] = 8;
	config_attrs[++i] = EGL_ALPHA_SIZE;
	config_attrs[++i] = 0;
	config_attrs[++i] = EGL_DEPTH_SIZE;
	config_attrs[++i] = 0;//16;
	config_attrs[++i] = EGL_STENCIL_SIZE;
	config_attrs[++i] = 0;
	config_attrs[++i] = EGL_NONE;
	
	FOUNDATION_ASSERT( i < MAX_CONFIG_ATTRS_SIZE );
	
	// choose configs
	EGLConfig matching_configs[MAX_MATCHING_CONFIGS];
	EGLint num_matching_configs = 0;
	if( !eglChooseConfig( drawable->display, config_attrs, matching_configs, MAX_MATCHING_CONFIGS, &num_matching_configs ) )
	{
		error_logf( ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, "Unable to find suitable EGL config: call failed (%s) (display %p)", eglGetErrorMessage( eglGetError() ), drawable->display );
		return false;
	}
	
	if( !num_matching_configs )
	{
		error_logf( ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, "Unable to find suitable EGL config: no matching config" );
		return false;
	}
	
	backend_gles2->config = matching_configs[0];
	
#if FOUNDATION_PLATFORM_ANDROID
	
	ANativeWindow_acquire( (ANativeWindow*)drawable->native );
	
	EGLint format = 0;
	eglGetConfigAttrib( drawable->display, backend_gles2->config, EGL_NATIVE_VISUAL_ID, &format );
	debug_logf( "Display config native visual ID: %d", format );
	
	unsigned int drawable_width = render_drawable_width( drawable );
	unsigned int drawable_height = render_drawable_height( drawable );
	ret = ANativeWindow_setBuffersGeometry( (ANativeWindow*)drawable->native, drawable_width, drawable_height, format );
	debug_logf( "Window setBuffersGeometry( %d, %d ) returned %d", drawable_width, drawable_height, ret );
	
	backend_gles2->surface = eglCreateWindowSurface( drawable->display, backend_gles2->config, (EGLNativeWindowType)drawable->native, 0 );
	if( !backend_gles2->surface )
	{
		error_logf( ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, "Unable to create EGL surface: %s", eglGetErrorMessage( eglGetError() ) );
		return false;
	}
	
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	
	DISPMANX_ELEMENT_HANDLE_T    dispman_element;
	DISPMANX_DISPLAY_HANDLE_T    dispman_display;
	DISPMANX_UPDATE_HANDLE_T     dispman_update;
	
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	
	uint32_t screen_width = 0;
	uint32_t screen_height = 0;
	int success = graphics_get_display_size( 0 /* LCD */, &screen_width, &screen_height);
	if( success < 0 )
	{
		error_logf( ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, "Unable to get screen dimensions: %d", success );
		return false;
	}
	
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = screen_width;
	dst_rect.height = screen_height;
	
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = screen_width << 16;
	src_rect.height = screen_height << 16;
	
	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );
	
	dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
											  0/*layer*/, &dst_rect, 0/*src*/,
											  &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/ );
	
	if( !drawable->native )
		drawable->native = allocate( render_allocator(), sizeof( EGL_DISPMANX_WINDOW_T ), 0 );
	EGL_DISPMANX_WINDOW_T* native_window = drawable->native;
	native_window->element = dispman_element;
	native_window->width   = screen_width;
	native_window->height  = screen_height;
	
	vc_dispmanx_update_submit_sync( dispman_update );
	
	backend_gles2->surface = eglCreateWindowSurface( drawable->display, backend_gles2->config, (EGLNativeWindowType)native_window, 0 );
	if( !backend_gles2->surface )
	{
		error_logf( ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, "Unable to create EGL surface: %s", eglGetErrorMessage( eglGetError() ) );
		return false;
	}
	
	drawable->width = screen_width;
	drawable->height = screen_height;
	
#endif
	
	EGLint context_attrs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	
	backend_gles2->context = eglCreateContext( drawable->display, backend_gles2->config, EGL_NO_CONTEXT, context_attrs );
	if( !backend_gles2->context )
	{
		error_logf( ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, "Unable to create EGL context: %s", eglGetErrorMessage( eglGetError() ) );
		return false;
	}
	
	eglMakeCurrent( drawable->display, backend_gles2->surface, backend_gles2->surface, backend_gles2->context );
	
	EGLint width = 0, height = 0;
	eglQuerySurface( drawable->display, backend_gles2->surface, EGL_WIDTH, &width );
	eglQuerySurface( drawable->display, backend_gles2->surface, EGL_HEIGHT, &height );
	
	backend_gles2->framebuffer_width = width;
	backend_gles2->framebuffer_height = height;
	
	debug_logf( "Window initialized for EGL rendering: 0x%p dimensions %dx%d", drawable->native, backend_gles2->framebuffer_width, backend_gles2->framebuffer_height );
	
	debug_logf( "Vendor     : %s", eglQueryString( drawable->display, EGL_VENDOR ) );
	debug_logf( "Version    : %s", eglQueryString( drawable->display, EGL_VERSION ) );
	debug_logf( "Extensions : %s", eglQueryString( drawable->display, EGL_EXTENSIONS ) );
	debug_logf( "Client APIs: %s", eglQueryString( drawable->display, EGL_CLIENT_APIS ) );
	
	//#if !FOUNDATION_BUILD_RTM
	int attributes[33] = {
		EGL_BUFFER_SIZE,
		EGL_ALPHA_SIZE,
		EGL_BLUE_SIZE,
		EGL_GREEN_SIZE,
		EGL_RED_SIZE,
		EGL_DEPTH_SIZE,
		EGL_STENCIL_SIZE,
		EGL_CONFIG_CAVEAT,
		EGL_CONFIG_ID,
		EGL_LEVEL,
		EGL_MAX_PBUFFER_HEIGHT,
		EGL_MAX_PBUFFER_PIXELS,
		EGL_MAX_PBUFFER_WIDTH,
		EGL_NATIVE_RENDERABLE,
		EGL_NATIVE_VISUAL_ID,
		EGL_NATIVE_VISUAL_TYPE,
		0x3030, //EGL_PRESERVED_RESOURCES,
		EGL_SAMPLES,
		EGL_SAMPLE_BUFFERS,
		EGL_SURFACE_TYPE,
		EGL_TRANSPARENT_TYPE,
		EGL_TRANSPARENT_RED_VALUE,
		EGL_TRANSPARENT_GREEN_VALUE,
		EGL_TRANSPARENT_BLUE_VALUE,
		EGL_BIND_TO_TEXTURE_RGB,
		EGL_BIND_TO_TEXTURE_RGBA,
		EGL_MIN_SWAP_INTERVAL,
		EGL_MAX_SWAP_INTERVAL,
		EGL_LUMINANCE_SIZE,
		EGL_ALPHA_MASK_SIZE,
		EGL_COLOR_BUFFER_TYPE,
		EGL_RENDERABLE_TYPE,
		EGL_CONFORMANT
	};
	const char* attribnames[33] = {
		"EGL_BUFFER_SIZE",
		"EGL_ALPHA_SIZE",
		"EGL_BLUE_SIZE",
		"EGL_GREEN_SIZE",
		"EGL_RED_SIZE",
		"EGL_DEPTH_SIZE",
		"EGL_STENCIL_SIZE",
		"EGL_CONFIG_CAVEAT",
		"EGL_CONFIG_ID",
		"EGL_LEVEL",
		"EGL_MAX_PBUFFER_HEIGHT",
		"EGL_MAX_PBUFFER_PIXELS",
		"EGL_MAX_PBUFFER_WIDTH",
		"EGL_NATIVE_RENDERABLE",
		"EGL_NATIVE_VISUAL_ID",
		"EGL_NATIVE_VISUAL_TYPE",
		"EGL_PRESERVED_RESOURCES",
		"EGL_SAMPLES",
		"EGL_SAMPLE_BUFFERS",
		"EGL_SURFACE_TYPE",
		"EGL_TRANSPARENT_TYPE",
		"EGL_TRANSPARENT_RED_VALUE",
		"EGL_TRANSPARENT_GREEN_VALUE",
		"EGL_TRANSPARENT_BLUE_VALUE",
		"EGL_BIND_TO_TEXTURE_RGB",
		"EGL_BIND_TO_TEXTURE_RGBA",
		"EGL_MIN_SWAP_INTERVAL",
		"EGL_MAX_SWAP_INTERVAL",
		"EGL_LUMINANCE_SIZE",
		"EGL_ALPHA_MASK_SIZE",
		"EGL_COLOR_BUFFER_TYPE",
		"EGL_RENDERABLE_TYPE",
		"EGL_CONFORMANT"
	};
	for( int ia = 0; ia < 33; ++ia )
	{
		EGLint value = 0;
		if( eglGetConfigAttrib( drawable->display, backend_gles2->config, attributes[ia], &value ) )
			debug_logf( "%s: %d", attribnames[ia], value );
		else
			debug_logf( "%s: <failed>", attribnames[ia] );
		eglGetError();
	}
	//#endif
	
#else
	FOUNDATION_ASSERT_FAIL( "Platform not implemented" );
	return error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );
#endif
	
	glBlendFunc( GL_ONE, GL_ZERO );
	glEnable( GL_BLEND );
	
	glLineWidth( 1.0f );
	
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glDepthMask( GL_TRUE );
	glDisable( GL_STENCIL_TEST );
	
	glStencilOpSeparate( GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP );
	glStencilMaskSeparate( GL_FRONT_AND_BACK, 0xFFFFFFFF );
	
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );
	glCullFace( GL_BACK );
	
	glHint( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
	
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	
	glViewport( 0, 0, backend_gles2->framebuffer_width, backend_gles2->framebuffer_height );
	
	_rb_gles2_check_error( "Error setting up default state" );
	
	return true;
}


static void                    _rb_gles2_dispatch( render_backend_t* backend, render_context_t** contexts, unsigned int num_contexts ) {}
static void                    _rb_gles2_flip( render_backend_t* backend ) { ++backend->framecount; }


static render_backend_vtable_t _render_backend_vtable_gles2 = {
	.construct = _rb_gles2_construct,
	.destruct  = _rb_gles2_destruct,
	.enumerate_adapters = _rb_gles2_enumerate_adapters,
	.enumerate_modes = _rb_gles2_enumerate_modes,
	.enable_thread = _rb_gles2_enable_thread,
	.disable_thread = _rb_gles2_disable_thread,
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
