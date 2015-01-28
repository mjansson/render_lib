/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOSX || ( FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI )

#include <render/gl4/glwrap.h>

#if RENDER_ENABLE_NVGLEXPERT
#  include <nvapi.h>

static void nvoglexpert_callback( unsigned int category, unsigned int id, unsigned int detail, int object, const char* msg )
{
	warn_logf( "nVidia OpenGL Expert error: Category 0x%08x, Message 0x%08x : %s", category, id, msg );
}

#endif

#include <render/gl4/glprocs.h>


typedef struct render_backend_gl4_t
{
	RENDER_DECLARE_BACKEND;

	void*                  context;
	render_resolution_t    resolution;
	
	bool                   use_clear_scissor;
} render_backend_gl4_t;


const char* _rb_gl_error_message( GLenum err )
{
	switch( err )
	{
		case GL_NONE:
			return "GL_NONE";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		default:
			break;
	}
	return "<UNKNOWN>";
}


bool _rb_gl_check_error( const char* message )
{
	GLenum err = glGetError();
	if( err != GL_NONE )
	{
		log_errorf( HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, "%s: %s", message, _rb_gl_error_message( err ) );
		FOUNDATION_ASSERT_FAILFORMAT( "OpenGL error: %s: %s", message, _rb_gl_error_message( err ) );
		return true;
	}
	return false;
}


#if !FOUNDATION_BUILD_DEPLOY

void STDCALL _rb_gl_debug_callback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam )
{
	log_debugf( HASH_RENDER, "OpenGL debug message: %s", message );
}

#endif
	

void _rb_gl_destroy_context( render_drawable_t* drawable, void* context )
{
#if FOUNDATION_PLATFORM_WINDOWS
	if( context )
	{
		if( wglGetCurrentContext() == context )
			wglMakeCurrent( 0, 0 );
		wglDeleteContext( (HGLRC)context );
	}
#elif FOUNDATOIN_PLATFORM_LINUX
	glXDestroyContext( drawable->display, context );
#elif FOUNDATION_PLATFORM_MACOSX
	_rb_gl_destroy_agl_context( context );
#endif	
}


void* _rb_gl_create_context( render_drawable_t* drawable, int major, int minor, void* share_context )
{
	if( drawable && ( drawable->type == RENDERDRAWABLE_OFFSCREEN ) )
	{
		log_errorf( HASH_RENDER, ERROR_NOT_IMPLEMENTED, "Offscreen drawable not implemented" );
		return 0;
	}

#if FOUNDATION_PLATFORM_WINDOWS

	int* attributes = 0;
	array_push( attributes, WGL_CONTEXT_MAJOR_VERSION_ARB ); array_push( attributes, major );
	array_push( attributes, WGL_CONTEXT_MINOR_VERSION_ARB ); array_push( attributes, minor );
	array_push( attributes, WGL_CONTEXT_FLAGS_ARB         ); array_push( attributes, 0 );
	array_push( attributes, WGL_CONTEXT_PROFILE_MASK_ARB  ); array_push( attributes, WGL_CONTEXT_CORE_PROFILE_BIT_ARB );
	array_push( attributes, 0 );
	
	HDC hdc = 0;
	HGLRC hglrc_default = 0;
	HGLRC hglrc = 0;

	PIXELFORMATDESCRIPTOR pfd;
	memset( &pfd, 0, sizeof( PIXELFORMATDESCRIPTOR ) );
	pfd.nSize        = sizeof( PIXELFORMATDESCRIPTOR );
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits   = 32;
	pfd.cDepthBits   = 24;
	pfd.cStencilBits = 0;
	pfd.iLayerType   = PFD_MAIN_PLANE;

	hdc = (HDC)drawable->hdc;
	NEO_ASSERT( hdc );

	int pixelformat = ChoosePixelFormat( hdc, &pfd );
	SetPixelFormat( hdc, pixelformat, &pfd );
	hglrc_default = wglCreateContext( hdc );
	NEO_ASSERT( hglrc_default );

	wglMakeCurrent( hdc, hglrc_default );

	//Create real context
	int err = 0;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)_rb_gl_get_proc_address( "wglCreateContextAttribsARB" );
	if( wglCreateContextAttribsARB )
		hglrc = wglCreateContextAttribsARB( hdc, (HGLRC)share_context, attributes );

	if( !hglrc && ( major < 3 ) )
	{
		hglrc = wglCreateContext( hdc );
		if( hglrc )
		{
			if( share_context )
			{
				wglShareLists( hglrc, (HGLRC)share_context );
				_rb_gl_check_error( "Unable to share GL render contexts" );
			}

			wglMakeCurrent( hdc, hglrc );

			const char* version = (const char*)glGetString( GL_VERSION );
			int have_major = 0, have_minor = 0, have_revision = 0;
			char** version_arr = string_explode( version, ".", false );
	
			have_major    = ( array_size( version_arr ) > 0 ) ? string_to_uint( version_arr[0], false ) : 0;
			have_minor    = ( array_size( version_arr ) > 1 ) ? string_to_uint( version_arr[1], false ) : 0;
			have_revision = ( array_size( version_arr ) > 2 ) ? string_to_uint( version_arr[2], false ) : 0;
	
			string_array_deallocate( version_arr );

			bool supported = ( have_major > major );
			if( !supported && ( ( have_major == major ) && ( have_minor >= minor ) ) )
				supported = true;
	
			if( !supported )
			{
				log_warnf( HASH_RENDER, WARNING_UNSUPPORTED, "GL version %d.%d not supported, got %d.%d (%s)", major, minor, have_major, have_minor, version );
				wglMakeCurrent( 0, 0 );
				wglDeleteContext( hglrc );
				hglrc = 0;
			}
		}
	}

	if( hglrc )
	{
		wglMakeCurrent( hdc, hglrc );
	}
	else
	{
		if( major >= 3 )
		{
			int err = GetLastError();
			log_infof( HASH_RENDER, "Unable to create GL context for version %d.%d: %s (%08x)", major, minor, system_error_message( err ), err );
		}
		wglMakeCurrent( 0, 0 );
	}
	wglDeleteContext( hglrc_default );
	
	array_deallocate( attributes );

	return hglrc;

#elif FOUNDATION_PLATFORM_LINUX

	GLXContext context = 0;
	GLXFBConfig* fbconfig;
	Display* display = 0;
	int screen = 0;
	int numconfig = 0;
	bool verify_only = false;

	if( !drawable )
	{
		display = XOpenDisplay( 0 );
		screen = DefaultScreen( display );
		verify_only = true;
	}
	else
	{
		display = drawable->display;
		screen = drawable->screen;
	}

	if( glXQueryExtension( display, 0, 0 ) != True )
	{
		log_warnf( HASH_RENDER, WARNING_UNSUPPORTED, "Unable to query GLX extension" );
		goto failed;
	}

	int major_glx = 0, minor_glx = 0;
	glXQueryVersion( display, &major_glx, &minor_glx );
	if( !verify_only )
		log_debugf( HASH_RENDER, "GLX version %d.%d", major_glx, minor_glx );
	
	fbconfig = glXGetFBConfigs( display, screen, &numconfig );
	if( !verify_only )
		log_debugf( HASH_RENDER, "Got %d configs", numconfig );
	if( fbconfig && ( numconfig > 0 ) )
	{
		PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribs = (PFNGLXCREATECONTEXTATTRIBSARBPROC)_rb_gl_get_proc_address( "glXCreateContextAttribsARB" );
		if( glXCreateContextAttribs )
		{
			int* attributes = 0;
			array_push( attributes, GLX_CONTEXT_MAJOR_VERSION_ARB ); array_push( attributes, major );
			array_push( attributes, GLX_CONTEXT_MINOR_VERSION_ARB ); array_push( attributes, minor );
			array_push( attributes, GLX_CONTEXT_FLAGS_ARB         ); array_push( attributes, 0 );
			array_push( attributes, GLX_CONTEXT_PROFILE_MASK_ARB  ); array_push( attributes, GLX_CONTEXT_CORE_PROFILE_BIT_ARB );
			array_push( attributes, 0 ); array_push( attributes, 0 );

			for( int ic = 0; ic < numconfig; ++ic )
			{
				context = glXCreateContextAttribs( display, fbconfig[ic], 0, true, attributes );
				if( context )
					break;
			}

			array_deallocate( attributes );
		}
		else
		{
			log_warnf( HASH_RENDER, WARNING_UNSUPPORTED, "Unable to get glXCreateContextAttribs proc address" );
		}
	}

failed:
	
	if( !drawable )
	{
		glXDestroyContext( display, context );
		XCloseDisplay( display );
	}
	
	return context;
	
#elif FOUNDATION_PLATFORM_MACOSX
	
	bool supported = false;
	
	CGDirectDisplayID display = CGMainDisplayID();
	void* view = ( drawable ? drawable->view : 0 );
	if( drawable )
	{
		//TODO: Get display mask from view
		//display = CGOpenGLDisplayMaskToDisplayID( _adapter._id );
	}
	unsigned int displaymask = CGDisplayIDToOpenGLDisplayMask( display );
	void* context = _rb_gl_create_agl_context( view, displaymask, 32/*color_depth*/, 24/*_res._depth*/, 8/*_res._stencil*/ );
	if( !context )
	{
		log_warnf( HASH_RENDER, WARNING_UNSUPPORTED, "Unable to create OpenGL context" );
		goto failed;
	}
	
	const char* version = (const char*)glGetString( GL_VERSION );
	int have_major = 0, have_minor = 0, have_revision = 0;
	char** version_arr = string_explode( version, ".", false );
	
	have_major    = ( array_size( version_arr ) > 0 ) ? string_to_uint( version_arr[0], false ) : 0;
	have_minor    = ( array_size( version_arr ) > 1 ) ? string_to_uint( version_arr[1], false ) : 0;
	have_revision = ( array_size( version_arr ) > 2 ) ? string_to_uint( version_arr[2], false ) : 0;
	
	string_array_deallocate( version_arr );

	supported = ( have_major > major );
	if( !supported && ( ( have_major == major ) && ( have_minor >= minor ) ) )
		supported = true;
	
	if( !supported )
	{
		log_infof( HASH_RENDER, "GL version %d.%d not supported, got %d.%d (%s)", major, minor, have_major, have_minor, version );
		goto failed;
	}
	
failed:
	
	if( ( !supported || !drawable ) && context )
		_rb_gl_destroy_agl_context( context );
		
	return supported ? context : 0;
	
#else
#  error Not implemented
#endif
}


bool _rb_gl_check_context( int major, int minor )
{
	void* context = 0;
	
#if FOUNDATION_PLATFORM_WINDOWS
	
	object_t window_check = window_create( 0, "__neo_gl_check", 10, 10, false );
	render_drawable_t* drawable = render_drawable_allocate();
	render_drawable_set_window( drawable, window_check );
	context = _create_gl_context( drawable, major, minor, 0 );
	window_destroy( window_check );
	render_drawable_deallocate( drawable );
	wglMakeCurrent( 0, 0 );
	if( context )
		wglDeleteContext( (HGLRC)context );
	
#elif FOUNDATION_PLATFORM_LINUX || FOUNDATION_PLATFORM_MACOSX
	
	context = _rb_gl_create_context( 0, major, minor, 0 );
		
#else
#  error Not implemented
#endif

	return ( context != 0 );
}


bool _rb_gl_check_extension( const char* name )
{
	return false;
}


static bool _rb_gl4_construct( render_backend_t* backend )
{
	//TODO: Caps check
	//if( !... )
	//  return false;

	log_debug( HASH_RENDER, "Constructed GL4 render backend" );
	return true;
}


static void _rb_gl4_destruct( render_backend_t* backend )
{
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;
	if( backend_gl4->context )
		_rb_gl_destroy_context( backend_gl4->drawable, backend_gl4->context );
	backend_gl4->context = 0;

	log_debug( HASH_RENDER, "Destructed GL4 render backend" );
}


static bool _rb_gl4_set_drawable( render_backend_t* backend, render_drawable_t* drawable )
{
	if( !FOUNDATION_VALIDATE_MSG( drawable->type != RENDERDRAWABLE_OFFSCREEN, "Offscreen drawable not implemented" ) )
		return error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );

	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;
	if( !FOUNDATION_VALIDATE_MSG( !backend_gl4->context, "Drawable switching not supported yet" ) )
		return error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );

	backend_gl4->context = _rb_gl_create_context( drawable, 4, 0, 0 );
	if( !backend_gl4->context )
	{
		log_error( HASH_RENDER, ERROR_UNSUPPORTED, "Unable to create OpenGL 4 context" );
		return false;
	}
	
#if FOUNDATION_PLATFORM_LINUX

	glXMakeCurrent( drawable->display, drawable->drawable, backend_gl4->context );
	
	if( True == glXIsDirect( drawable->display, backend_gl4->context ) )
		log_debug( HASH_RENDER, "Direct rendering enabled" );
	else
		log_warn( HASH_RENDER, WARNING_PERFORMANCE, "Indirect rendering" );	

#endif

#if RENDER_ENABLE_NVGLEXPERT
	NvAPI_OGL_ExpertModeSet( 20, NVAPI_OGLEXPERT_REPORT_ALL, NVAPI_OGLEXPERT_OUTPUT_TO_CALLBACK, nvoglexpert_callback );
#endif

#if BUILD_ENABLE_LOG
	const char* vendor   = (const char*)glGetString( GL_VENDOR     );
	const char* renderer = (const char*)glGetString( GL_RENDERER   );
	const char* version  = (const char*)glGetString( GL_VERSION    );
	const char* ext      = (const char*)glGetString( GL_EXTENSIONS );

	log_infof( HASH_RENDER, "Vendor:     %s", vendor ? vendor : "<unknown>" );
	log_infof( HASH_RENDER, "Renderer:   %s", renderer ? renderer : "<unknown>" );
	log_infof( HASH_RENDER, "Version:    %s", version ? version : "<unknown>" );
	log_infof( HASH_RENDER, "Extensions: %s", ext ? ext : "<none>" );
#endif

#if FOUNDATION_PLATFORM_WINDOWS
	const char* wglext = 0;
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = 0;
	if( ( wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)glGetProcAddress( "wglGetExtensionsStringARB" ) ) != 0 )
		wglext = wglGetExtensionsStringARB( (HDC)drawable->hdc );
	else if( ( wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)glGetProcAddress( "wglGetExtensionsStringEXT" ) ) != 0 )
		wglext = wglGetExtensionsStringEXT();
	log_debugf( HASH_RENDER, "WGL Extensions: %s", wglext ? wglext : "<none>" );
#endif

	if( !_rb_gl_get_standard_procs( 4, 0 ) )
		return false;

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	glReadBuffer( GL_BACK );
	glDrawBuffer( GL_BACK );

	glViewport( 0, 0, render_drawable_width( drawable ), render_drawable_height( drawable ) );
	
	_rb_gl_check_error( "Error setting up default state" );
	
	return true;
}


FOUNDATION_DECLARE_THREAD_LOCAL( void*, gl4_context, 0 )

static void _rb_gl4_enable_thread( render_backend_t* backend )
{
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	void* thread_context = get_thread_gl4_context();
	if( !thread_context )
	{
		thread_context = _rb_gl_create_context( backend->drawable, 4, 0, backend_gl4->context );
		set_thread_gl4_context( thread_context );
	}

#if NEO_PLATFORM_WINDOWS
	if( !wglMakeCurrent( (HDC)backend->drawable->hdc, (HGLRC)thread_context ) )
		_rb_gl_check_error( "Unable to enable thread for rendering" );
	else
		log_debug( HASH_RENDER, "Enabled thread for GL4 rendering" );
#elif NEO_PLATFORM_LINUX
	glXMakeCurrent( backend->drawable->display, backend->drawable->drawable, thread_context );
	_rb_gl_check_error( "Unable to enable thread for rendering" );
#else
	FOUNDATION_ASSERT_FAIL( "Platform not implemented" );
	error_report( ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED );
#endif
}


static void _rb_gl4_disable_thread( render_backend_t* backend )
{
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	void* thread_context = get_thread_gl4_context();
	if( thread_context )
	{
		_rb_gl_destroy_context( backend_gl4->drawable, thread_context );
		log_debug( HASH_RENDER, "Disabled thread for GL4 rendering" );
	}
	set_thread_gl4_context( 0 );
}


unsigned int* _rb_gl4_enumerate_adapters( render_backend_t* backend )
{
	unsigned int* adapters = 0;
	array_push( adapters, WINDOW_ADAPTER_DEFAULT );
	return adapters;
}


render_resolution_t* _rb_gl4_enumerate_modes( render_backend_t* backend, unsigned int adapter )
{
	render_resolution_t* modes = 0;

#if FOUNDATION_PLATFORM_LINUX

	FOUNDATION_ASSERT_MSG( adapter == WINDOW_ADAPTER_DEFAULT, "render_enumerate_modes not implemented when adapter is specified" );

	Display* display = XOpenDisplay( 0 );
	if( !display )
	{
		log_error( HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, "Unable to enumerate modes, unable to open display" );
		goto exit;
	}

	int depths[] = { 15, 16, 24, 32 };
	for( int i = 0; i < 4; ++i )
	{
		int num = 0;
		XVisualInfo info;
		memset( &info, 0, sizeof( XVisualInfo ) );
		info.depth = depths[i];
	
		XVisualInfo* visual = XGetVisualInfo( display, VisualDepthMask, &info, &num );
		for( int v = 0; v < num; ++v )
		{
			XF86VidModeModeInfo** xmodes = 0;
			int nummodes = 0;

			int depth = 0, stencil = 0, color = 0;
			glXGetConfig( display, &visual[v], GLX_BUFFER_SIZE,  &color   );
			glXGetConfig( display, &visual[v], GLX_DEPTH_SIZE,   &depth   );
			glXGetConfig( display, &visual[v], GLX_STENCIL_SIZE, &stencil );

			if( ( color < 24 ) || ( depth < 15 ) )
				continue;
			
			XF86VidModeGetAllModeLines( display, visual[v].screen, &nummodes, &xmodes );
			/*if( nummodes )
			qsort( xmodes, nummodes, sizeof( XF86VidModeModeInfo* ), Adapter::compareModes );*/

			for( int m = 0; m < nummodes; ++m )
			{
				if( ( xmodes[m]->hdisplay < 600 ) || ( xmodes[m]->vdisplay < 400 ) )
					continue;

				int refresh =  ( (int)( 0.5f + ( 1000.0f * xmodes[m]->dotclock ) / (float)( xmodes[m]->htotal * xmodes[m]->vtotal ) ) );

				if( XF86VidModeValidateModeLine( display, visual[v].screen, xmodes[m] ) == 255 ) // 255 = MODE_BAD according to XFree sources...
					continue;

				pixelformat_t format = PIXELFORMAT_R8G8B8A8;
				if( color == 24 )
					format = PIXELFORMAT_R8G8B8;

				resolution_t mode = {
					0,
					xmodes[m]->hdisplay,
					xmodes[m]->vdisplay,
					format,
					COLORSPACE_LINEAR,
					refresh
				};

				bool found = false;
				for( int c = 0, size = array_size( modes ); c < size; ++c )
				{
					if( !memcmp( modes + c, &mode, sizeof( resolution_t ) ) )
					{
						found = true;
						break;
					}
				}
				if( !found )
					array_push( modes, mode );
			}

			XFree( xmodes );
		}

		XFree( visual );
	}

	//Sort and index modes
	for( int c = 0, size = array_size( modes ); c < size; ++c )
	{
		modes[c].id = c;
	}
	
	if( !array_size( modes ) )
	{
		log_warnf( HASH_RENDER, WARNING_SUSPICIOUS, "Unable to enumerate resolutions for adapter %d, adding default fallback", adapter );
		render_resolution_t mode = {
			0,
			800,
			600,
			PIXELFORMAT_R8G8B8X8,
			COLORSPACE_LINEAR,
			60
		};
		array_push( modes, mode );
	}

exit:

	XCloseDisplay( display );
	
#else	

	render_resolution_t mode = {
		0,
		800,
		600,
		PIXELFORMAT_R8G8B8X8,
		60
	};
	array_push( modes, mode );

#endif

	return modes;
}


static void* _rb_gl4_allocate_buffer( render_backend_t* backend, render_buffer_t* buffer )
{
	return memory_allocate( HASH_RENDER, buffer->size * buffer->allocated, 16, MEMORY_PERSISTENT );
}


static void _rb_gl4_deallocate_buffer( render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux )
{
	if( sys )
		memory_deallocate( buffer->store );
	
	if( aux && buffer->backend_data[0] )
	{
		GLuint buffer_object = (GLuint)buffer->backend_data[0];
		glDeleteBuffers( 1, &buffer_object );
		buffer->backend_data[0] = 0;
	}
}


static void _rb_gl4_upload_buffer( render_backend_t* backend, render_buffer_t* buffer )
{
	GLuint buffer_object = (GLuint)buffer->backend_data[0];
	if( !buffer_object )
	{
		glGenBuffers( 1, &buffer_object );
		_rb_gl_check_error( "Unable to create buffer object" );
		buffer->backend_data[0] = buffer_object;
	}
	
	glBindBuffer( GL_ARRAY_BUFFER, buffer_object );
	glBufferData( GL_ARRAY_BUFFER, buffer->size * buffer->allocated, buffer->store, ( buffer->usage == RENDERUSAGE_DYNAMIC ) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW );
	_rb_gl_check_error( "Unable to upload buffer object data" );
	
	buffer->flags &= ~RENDERBUFFER_DIRTY;
}


static void _rb_gl4_upload_shader( render_backend_t* backend, render_shader_t* shader, const void* buffer, unsigned int size )
{
	//render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	switch( shader->shadertype )
	{
		case SHADER_VERTEX:
		{
			//Vertex shader backend data:
			//  0 - Shader object
			/*if( shader->backend_data[0] )
				glDeleteObject( shader->backend_data[0] );
			shader->backend_data[0] = 0;

			GLuint handle = glCreateShaderObject( GL_VERTEX_SHADER_ARB );
			GLchar* source = (GLchar*)buffer;
			GLint source_size = size;
			glShaderSource( handle, &buffer, &size );
			glCompileShader( handle );

			GLint compiled = 0;
			glGetObjectParameteriv( handle, GL_OBJECT_COMPILE_STATUS_ARB, &compiled );

			if( !compiled )
			{
				//...
				glDeleteObject( handle );
			}
			else
			{
				shader->backend_data[0] = handle;
			}*/

			break;
		}

		default:
			break;
	}
}


static void _rb_gl4_deallocate_shader( render_backend_t* backend, render_shader_t* shader )
{
	/*if( shader->backend_data[0] )
		glDeleteObject( shader->backend_data[0] );
	shader->backend_data[0] = 0;*/
}


#if 0
static render_parameter_block_t* _rb_gl4_allocate_parameter_block( render_backend_t* backend, const render_parameter_t* parameters, unsigned int num )
{
	render_parameter_block_t* block = memory_allocate( HASH_RENDER, sizeof( render_parameter_block_t ) + ( sizeof( render_parameter_info_t ) * num ), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZE );
	for( unsigned int i = 0; i < num; ++i )
	{
		block->info[i].type   = parameters[i].type;
		block->info[i].dim    = parameters[i].dim;
		block->info[i].offset = 0;
	}
	return block;
}


static void _rb_gl4_bind_parameter_block( render_backend_t* backend, render_parameter_block_t* block, unsigned int index, const void* RESTRICT data )
{
}


static void _rb_gl4_deallocate_parameter_block( render_backend_t* backend, render_parameter_block_t* block )
{
	memory_deallocate( block );
}


static void _rb_gl4_upload_texture( render_backend_t* backend, render_texture_t* texture, unsigned int width, unsigned int height, unsigned int depth, unsigned int levels, const void* data )
{
}


static render_texture_t* _rb_gl4_allocate_texture( render_backend_t* backend, render_texture_type_t type, render_usage_t usage, pixelformat_t format, colorspace_t colorspace )
{
	render_texture_t* texture = memory_allocate( HASH_RENDER, sizeof( render_texture_t ), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZE );
	texture->textype = type;
	texture->usage = usage;
	texture->format = format;
	texture->colorspace = colorspace;
	return texture;
}


static void _rb_gl4_deallocate_texture( render_backend_t* backend, render_texture_t* texture )
{
	memory_deallocate( texture );
}
#endif


static void _rb_gl4_dispatch( render_backend_t* backend, render_context_t** contexts, unsigned int num_contexts )
{
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	for( int context_index = 0, context_size = num_contexts; context_index < context_size; ++context_index )
	{
		render_context_t* context = contexts[context_index];
		render_command_t* command = context->commands;
		const radixsort_index_t* order = context->order;

		for( int cmd_index = 0, cmd_size = atomic_load32( &context->reserved ); cmd_index < cmd_size; ++cmd_index, ++order ) { command = context->commands + *order; switch( command->type )
		{
			case RENDERCOMMAND_CLEAR:
			{
				unsigned int buffer_mask = command->data.clear.buffer_mask;
				unsigned int bits = 0;
					
				if( buffer_mask & RENDERBUFFER_COLOR )
				{
					unsigned int color_mask = command->data.clear.color_mask;
					uint32_t color = command->data.clear.color;
					glColorMask( ( color_mask & 0x01 ) ? GL_TRUE : GL_FALSE, ( color_mask & 0x02 ) ? GL_TRUE : GL_FALSE, ( color_mask & 0x04 ) ? GL_TRUE : GL_FALSE, ( color_mask & 0x08 ) ? GL_TRUE : GL_FALSE );
					bits |= GL_COLOR_BUFFER_BIT;
					//color_linear_t color = uint32_to_color( command->data.clear.color );
					//glClearColor( vector_x( color ), vector_y( color ), vector_z( color ), vector_w( color ) );
					glClearColor( (float)( color & 0xFF ) / 255.0f, (float)( ( color >> 8 ) & 0xFF ) / 255.0f, (float)( ( color >> 16 ) & 0xFF ) / 255.0f, (float)( ( color >> 24 ) & 0xFF ) / 255.0f );
				}
					
				if( buffer_mask & RENDERBUFFER_DEPTH )
				{
					glDepthMask( GL_TRUE );
					bits |= GL_DEPTH_BUFFER_BIT;
					glClearDepth( command->data.clear.depth );
				}
					
				if( buffer_mask & RENDERBUFFER_STENCIL )
				{
					glClearStencil( command->data.clear.stencil );
					bits |= GL_STENCIL_BUFFER_BIT;
				}
				
				if( backend_gl4->use_clear_scissor )
					glEnable( GL_SCISSOR_TEST );
				
				glClear( bits );

				if( backend_gl4->use_clear_scissor )
					glDisable( GL_SCISSOR_TEST );
				glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

				break;
			}

			case RENDERCOMMAND_VIEWPORT:
			{
				int target_width = render_target_width( context->target );
				int target_height = render_target_height( context->target );
				
				GLint x = command->data.viewport.x;
				GLint y = command->data.viewport.y;
				GLsizei w = command->data.viewport.width;
				GLsizei h = command->data.viewport.height;

				glViewport( x, y, w, h );
				glScissor( x, y, w, h );
				
				backend_gl4->use_clear_scissor = ( x || y || ( w != target_width ) || ( h != target_height ) );
				break;
			}
		} }
	}	
}


static void _rb_gl4_flip( render_backend_t* backend )
{
	//render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

#if FOUNDATION_PLATFORM_WINDOWS

	/*if( _hdc )
	{
		if( !SwapBuffers( _hdc ) )
			core::Core::get()->error( core::ERROR, "render::opengl::Device", "flip", "Unable to flip buffers: " + core::Core::getWindowsErrorMessage() );
	}*/

#elif FOUNDATION_PLATFORM_MACOSX

	/*if( _fullscreen && _context )
		CGLFlushDrawable( _context );
	else
	window::objc::flushDrawable( (void*)_context );*/
	
#elif FOUNDATION_PLATFORM_LINUX

	if( backend_gl4->drawable->display )
		glXSwapBuffers( backend_gl4->drawable->display, backend_gl4->drawable->drawable );
	
#else
#  error Not implemented
#endif

	++backend->framecount;
}


static render_backend_vtable_t _render_backend_vtable_gl4 = {
	.construct = _rb_gl4_construct,
	.destruct  = _rb_gl4_destruct,
	.enumerate_adapters = _rb_gl4_enumerate_adapters,
	.enumerate_modes = _rb_gl4_enumerate_modes,
	.set_drawable = _rb_gl4_set_drawable,
	.enable_thread = _rb_gl4_enable_thread,
	.disable_thread = _rb_gl4_disable_thread,
	.allocate_buffer = _rb_gl4_allocate_buffer,
	.deallocate_buffer = _rb_gl4_deallocate_buffer,
	.upload_buffer = _rb_gl4_upload_buffer,
	.upload_shader = _rb_gl4_upload_shader,
	.deallocate_shader = _rb_gl4_deallocate_shader,
	/*.allocate_parameter_block = _rb_gl4_allocate_parameter_block,
	.bind_parameter_block = _rb_gl4_bind_parameter_block,
	.deallocate_parameter_block = _rb_gl4_deallocate_parameter_block,
	.allocate_texture = _rb_gl4_allocate_texture,
	.upload_texture = _rb_gl4_upload_texture,
	.deallocate_texture = _rb_gl4_deallocate_texture,*/
	.dispatch = _rb_gl4_dispatch,
	.flip = _rb_gl4_flip
};


render_backend_t* render_backend_gl4_allocate()
{
	if( !_rb_gl_check_context( 4, 0 ) )
		return 0;

	log_debug( HASH_RENDER, "Allocating GL4 render backend" );

#if NEO_ENABLE_NVGLEXPERT
	static bool nvInitialized = false;
	if( !nvInitialized )
	{
		nvInitialized = true;
		NvAPI_Initialize();
	}
#endif

	render_backend_gl4_t* backend = memory_allocate( HASH_RENDER, sizeof( render_backend_gl4_t ), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED );
	backend->api = RENDERAPI_OPENGL4;
	backend->vtable = _render_backend_vtable_gl4;
	return (render_backend_t*)backend;
}

#else

render_backend_t* render_gl4_allocate()
{
	return 0;
}

#endif
