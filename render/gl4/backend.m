/* backend.m  -  Render library  -  Public Domain  -  2014 Mattias Jansson
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/render_lib
 *
 * The dependent library source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#if FOUNDATION_PLATFORM_MACOS

#include <foundation/apple.h>
#include <render/hashstrings.h>
#include <render/gl4/glwrap.h>

#import <Foundation/NSIndexPath.h>
#import <Foundation/NSLock.h>
#import <AppKit/NSView.h>
#import <AppKit/NSOpenGL.h>
#import <OpenGL/OpenGL.h>

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic push
#if __has_warning("-Wdeprecated-declarations")
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

void*
rb_gl_create_agl_context(void* view, unsigned int displaymask, unsigned int color, unsigned int depth,
                         unsigned int stencil, void* pixelformat) {
	FOUNDATION_UNUSED(pixelformat);

	NSView* nsview = (__bridge NSView*)view;

	NSOpenGLPixelFormatAttribute attribs[] = {NSOpenGLPFAScreenMask,
	                                          displaymask,
	                                          NSOpenGLPFADoubleBuffer,
	                                          NSOpenGLPFAMinimumPolicy,
	                                          NSOpenGLPFAWindow,
	                                          NSOpenGLPFANoRecovery,
	                                          NSOpenGLPFAColorSize,
	                                          (color > 16) ? color : 16,
	                                          NSOpenGLPFADepthSize,
	                                          (depth > 15) ? depth : 15,
	                                          NSOpenGLPFAStencilSize,
	                                          stencil,
	                                          0};

	if (!stencil)
		attribs[10] = attribs[11] = 0;

	NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	if (!format)
		return 0;

	NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
	if (view)
		[context setView:nsview];
	[context makeCurrentContext];

	// void* context_cgl = [context CGLContextObj];
	//*pixelformat = [format CGLPixelFormatObj];

	log_infof(HASH_RENDER, STRING_CONST("Created gl context %" PRIfixPTR " for view %" PRIfixPTR), (uintptr_t)context,
	          (uintptr_t)view);

	return (__bridge_retained void*)context;
}

void
rb_gl_agl_make_context_current(void* context) {
	if (!context)
		[NSOpenGLContext clearCurrentContext];
	else
		[(__bridge NSOpenGLContext*)context makeCurrentContext];
}

void*
rb_gl_agl_context_current(void) {
	NSOpenGLContext* context = [NSOpenGLContext currentContext];
	return (__bridge void*)context;
}

void
rb_gl_destroy_agl_context(void* context) {
	if (!context)
		return;

	NSOpenGLContext* last_context = (__bridge_transfer NSOpenGLContext*)context;
	NSOpenGLContext* current_context = [NSOpenGLContext currentContext];
	if (current_context == last_context)
		[NSOpenGLContext clearCurrentContext];
	[last_context clearDrawable];
}

void
rb_gl_flush_drawable(void* context) {
	if (context)
		[(__bridge NSOpenGLContext*)context flushBuffer];
}

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic pop
#endif

#endif
