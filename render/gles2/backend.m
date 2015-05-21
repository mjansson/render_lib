/* backend.m  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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


#if FOUNDATION_PLATFORM_IOS

#include <foundation/apple.h>

#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>


const void* _rb_gles2_ios_create_egl_context( void )
{
	EAGLContext* context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	if( !context )
	{
		log_warn( HASH_RENDER, WARNING_UNSUPPORTED, "Unable to allocate GLES2 EAGL context" );
		return 0;
	}

	[EAGLContext setCurrentContext:context];

	unsigned int major, minor;
	EAGLGetVersion( &major, &minor);

	log_debugf( HASH_RENDER, "Initialized GLES2 EAGL context %p version %d.%d", context, major, minor );

	return CFBridgingRetain(context);
}


void _rb_gles2_ios_set_current_egl_context( const void* context )
{
	[EAGLContext setCurrentContext:(__bridge EAGLContext*)context];
}


void _rb_gles2_ios_destroy_egl_context( const void* context )
{
	[EAGLContext setCurrentContext:0];
    CFBridgingRelease( context );
}


int _rb_gles2_ios_screen_width( void )
{
	return (int)[UIScreen mainScreen].currentMode.size.width;
}


int _rb_gles2_ios_screen_height( void )
{
	return (int)[UIScreen mainScreen].currentMode.size.height;
}


bool _rb_gles2_ios_render_buffer_storage_from_drawable( const void* context, const void* drawable, unsigned int framebuffer, unsigned int colorbuffer )
{
	EAGLContext* eagl_context = (__bridge EAGLContext*)context;
	CAEAGLLayer* layer = (__bridge CAEAGLLayer*)drawable;

	[EAGLContext setCurrentContext:eagl_context];

	glBindRenderbuffer( GL_RENDERBUFFER, colorbuffer );

	if( ![eagl_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer] )
	{
		log_errorf( HASH_RENDER, ERROR_OUT_OF_MEMORY, "Unable to allocate render buffer storage" );
		return false;
	}

	int backing_width = 0, backing_height = 0;
	glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backing_width );
    glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backing_height );
	log_debugf( HASH_RENDER, "Context 0x%" PRIfixPTR ", drawable 0x%" PRIfixPTR ", colorbuffer %d storage allocated for drawable size %dx%d", context, drawable, colorbuffer, backing_width, backing_height );

	return true;
}


void _rb_gles2_ios_present_render_buffer( const void* context )
{
	EAGLContext* eagl_context = (__bridge EAGLContext*)context;

	[EAGLContext setCurrentContext:eagl_context];

	if( ![eagl_context presentRenderbuffer:GL_RENDERBUFFER] )
		log_warnf( HASH_RENDER, WARNING_SUSPICIOUS, "Failed presentRenderbuffer" );
}


#endif
