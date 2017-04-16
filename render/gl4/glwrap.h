/* glwrap.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#pragma once

#ifndef STDCALL
#  define STDCALL
#endif

#if FOUNDATION_PLATFORM_WINDOWS

#  include <foundation/windows.h>

#  include <gl/GL.h>

#  include "gl/glext.h"
#  include "gl/wglext.h"

#elif FOUNDATION_PLATFORM_MACOS

#  include <foundation/apple.h>
#  include <render/types.h>

#  include <OpenGL/OpenGL.h>
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
#  include <AGL/agl.h>
#  include <mach-o/dyld.h>
#  include <dlfcn.h>

//Objective-C interface
RENDER_EXTERN void*
_rb_gl_create_agl_context(void* view, unsigned int displaymask, unsigned int color,
                          unsigned int depth, unsigned int stencil, void* pixelformat);

RENDER_EXTERN void
_rb_gl_destroy_agl_context(void* context);

RENDER_EXTERN void
_rb_gl_make_agl_context_current(void* context);

RENDER_EXTERN void
_rb_gl_flush_drawable(void* context);

#elif FOUNDATION_PLATFORM_LINUX

#  include <foundation/posix.h>
#  include <X11/Xlib.h>
#  include <X11/extensions/xf86vmode.h>
#  include <GL/gl.h>
#  include <GL/glx.h>
#  include <GL/glext.h>

#endif
