/* backend.m  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include <foundation/apple.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>


const void* _rb_gles2_create_egl_context( void )
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


void _rb_gles2_destroy_egl_context( const void* context )
{
	[EAGLContext setCurrentContext:0];
    CFBridgingRelease( context );
}


int _rb_gles2_screen_width( void )
{
	return (int)[UIScreen mainScreen].currentMode.size.width;
}


int _rb_gles2_screen_height( void )
{
	return (int)[UIScreen mainScreen].currentMode.size.height;
}


#endif
