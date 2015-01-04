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

#include <foundation/platform.h>

#if FOUNDATION_PLATFORM_MACOSX

#include <foundation/apple.h>
#include <render/hashstrings.h>

#import <Foundation/NSIndexPath.h>
#import <Foundation/NSLock.h>
#import <AppKit/NSView.h>
#import <AppKit/NSOpenGL.h>
#import <OpenGL/OpenGL.h>


void* _rb_gl_create_agl_context( void* view, unsigned int displaymask, unsigned int color, unsigned int depth, unsigned int stencil, void** pixelformat )
{
	NSView* nsview = (__bridge NSView*)view;

	NSOpenGLPixelFormatAttribute attribs[] = {
		NSOpenGLPFAScreenMask,      displaymask,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAMinimumPolicy,
		NSOpenGLPFAWindow,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAColorSize,       ( color > 16 ) ? color : 16,
		NSOpenGLPFADepthSize,       ( depth > 15 ) ? depth : 15,
		NSOpenGLPFAStencilSize,     stencil,
		0 };
	
	if( !stencil )
		attribs[ 10 ] = attribs[ 11 ] = 0;
	
	NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	if( !format )
		return 0;
	
	NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
	if( view )
		[context setView:nsview];
	[context makeCurrentContext];
		
	//void* context_cgl = [context CGLContextObj];
	//*pixelformat = [format CGLPixelFormatObj]; 
	
	log_infof( HASH_RENDER, "Created gl context %p for view %p", context, view );
	
	return (__bridge_retained void*)context;
}


void _rb_gl_make_agl_context_current( void* context )
{
	if( !context )
		[NSOpenGLContext clearCurrentContext];
	else
		[(__bridge NSOpenGLContext*)context makeCurrentContext];
}


void _rb_gl_destroy_agl_context( void* context )
{
	if( !context )
		return;
	
	NSOpenGLContext* last_context = (__bridge_transfer NSOpenGLContext*)context;
	NSOpenGLContext* current_context = [NSOpenGLContext currentContext];
	if( current_context == last_context )
		[NSOpenGLContext clearCurrentContext];
	[last_context clearDrawable];
}


void _rb_gl_flush_drawable( void* context )
{
	if( context )
		[(__bridge NSOpenGLContext*)context flushBuffer];
}


#endif
