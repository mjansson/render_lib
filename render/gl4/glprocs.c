/* glprocs.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <foundation/log.h>

#include <render/backend.h>
#include <render/hashstrings.h>

#include <window/window.h>

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOS || \
    (FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI)

#include <render/gl4/glwrap.h>
#include <render/gl4/glprocs.h>

#if RENDER_ENABLE_NVGLEXPERT
#include <nvapi.h>
#endif

glfn
_rb_gl_get_proc_address(const char* name) {
#if FOUNDATION_PLATFORM_WINDOWS
	return (glfn)wglGetProcAddress(name);
#elif FOUNDATION_PLATFORM_LINUX
	return (glfn)glXGetProcAddressARB((const GLubyte*)name);
#elif FOUNDATION_PLATFORM_MACOS
	return (glfn)dlsym(RTLD_DEFAULT, name);
#else
#error Not implemented
#endif
}

#ifndef GL_GLEXT_PROTOTYPES

PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLSAMPLECOVERAGEPROC glSampleCoverage;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glCompressedTexImage1D;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glGetCompressedTexImage;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;

PFNGLGENQUERIESPROC glGenQueries;
PFNGLDELETEQUERIESPROC glDeleteQueries;
PFNGLISQUERYPROC glIsQuery;
PFNGLBEGINQUERYPROC glBeginQuery;
PFNGLENDQUERYPROC glEndQuery;
PFNGLGETQUERYIVPROC glGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;

PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLISBUFFERPROC glIsBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv;

PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate;
PFNGLDRAWBUFFERSPROC glDrawBuffers;
PFNGLSTENCILOPSEPARATEPROC glStencilOpSeparate;
PFNGLSTENCILFUNCSEPARATEPROC glStencilFuncSeparate;
PFNGLSTENCILMASKSEPARATEPROC glStencilMaskSeparate;

PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders;

PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETSHADERSOURCEPROC glGetShaderSource;
PFNGLISPROGRAMPROC glIsProgram;
PFNGLISSHADERPROC glIsShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;

PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORM4IVPROC glUniform4iv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLGETUNIFORMFVPROC glGetUniformfv;
PFNGLGETUNIFORMIVPROC glGetUniformiv;

PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;

PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLGETVERTEXATTRIBDVPROC glGetVertexAttribdv;
PFNGLGETVERTEXATTRIBFVPROC glGetVertexAttribfv;
PFNGLGETVERTEXATTRIBIVPROC glGetVertexAttribiv;
PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;

PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;

PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
#endif

bool
_rb_gl_get_texture_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)_rb_gl_get_proc_address("glActiveTexture");
	glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)_rb_gl_get_proc_address("glSampleCoverage");
	glCompressedTexImage3D =
	    (PFNGLCOMPRESSEDTEXIMAGE3DPROC)_rb_gl_get_proc_address("glCompressedTexImage3D");
	glCompressedTexImage2D =
	    (PFNGLCOMPRESSEDTEXIMAGE2DPROC)_rb_gl_get_proc_address("glCompressedTexImage2D");
	glCompressedTexImage1D =
	    (PFNGLCOMPRESSEDTEXIMAGE1DPROC)_rb_gl_get_proc_address("glCompressedTexImage1D");
	glCompressedTexSubImage3D =
	    (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)_rb_gl_get_proc_address("glCompressedTexSubImage3D");
	glCompressedTexSubImage2D =
	    (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)_rb_gl_get_proc_address("glCompressedTexSubImage2D");
	glCompressedTexSubImage1D =
	    (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)_rb_gl_get_proc_address("glCompressedTexSubImage1D");
	glGetCompressedTexImage =
	    (PFNGLGETCOMPRESSEDTEXIMAGEPROC)_rb_gl_get_proc_address("glGetCompressedTexImage");
	if (!glActiveTexture || !glSampleCoverage || !glCompressedTexImage3D ||
	    !glCompressedTexImage2D || !glCompressedTexImage1D || !glCompressedTexSubImage3D ||
	    !glCompressedTexSubImage2D || !glCompressedTexSubImage1D || !glGetCompressedTexImage) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED,
		          STRING_CONST("Unable to get GL procs for textures"));
		return false;
	}
	glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)_rb_gl_get_proc_address("glGenerateMipmap");
#endif
	return true;
}

bool
_rb_gl_get_query_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glGenQueries = (PFNGLGENQUERIESPROC)_rb_gl_get_proc_address("glGenQueries");
	glDeleteQueries = (PFNGLDELETEQUERIESPROC)_rb_gl_get_proc_address("glDeleteQueries");
	glIsQuery = (PFNGLISQUERYPROC)_rb_gl_get_proc_address("glIsQuery");
	glBeginQuery = (PFNGLBEGINQUERYPROC)_rb_gl_get_proc_address("glBeginQuery");
	glEndQuery = (PFNGLENDQUERYPROC)_rb_gl_get_proc_address("glEndQuery");
	glGetQueryiv = (PFNGLGETQUERYIVPROC)_rb_gl_get_proc_address("glGetQueryiv");
	glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)_rb_gl_get_proc_address("glGetQueryObjectiv");
	glGetQueryObjectuiv =
	    (PFNGLGETQUERYOBJECTUIVPROC)_rb_gl_get_proc_address("glGetQueryObjectuiv");
	if (!glGenQueries || !glDeleteQueries || !glIsQuery || !glBeginQuery || !glEndQuery ||
	    !glGetQueryiv || !glGetQueryObjectiv || !glGetQueryObjectuiv) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED,
		          STRING_CONST("Unable to get GL procs for queries"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_buffer_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glBindBuffer = (PFNGLBINDBUFFERPROC)_rb_gl_get_proc_address("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)_rb_gl_get_proc_address("glDeleteBuffers");
	glGenBuffers = (PFNGLGENBUFFERSPROC)_rb_gl_get_proc_address("glGenBuffers");
	glIsBuffer = (PFNGLISBUFFERPROC)_rb_gl_get_proc_address("glIsBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)_rb_gl_get_proc_address("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)_rb_gl_get_proc_address("glBufferSubData");
	glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)_rb_gl_get_proc_address("glGetBufferSubData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)_rb_gl_get_proc_address("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)_rb_gl_get_proc_address("glUnmapBuffer");
	glGetBufferParameteriv =
	    (PFNGLGETBUFFERPARAMETERIVPROC)_rb_gl_get_proc_address("glGetBufferParameteriv");
	glGetBufferPointerv =
	    (PFNGLGETBUFFERPOINTERVPROC)_rb_gl_get_proc_address("glGetBufferPointerv");
	if (!glBindBuffer || !glDeleteBuffers || !glGenBuffers || !glIsBuffer || !glBufferData ||
	    !glBufferSubData || !glGetBufferSubData || !glMapBuffer || !glUnmapBuffer ||
	    !glGetBufferParameteriv || !glGetBufferPointerv) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED,
		          STRING_CONST("Unable to get GL procs for buffers"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_shader_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glBlendEquationSeparate =
	    (PFNGLBLENDEQUATIONSEPARATEPROC)_rb_gl_get_proc_address("glBlendEquationSeparate");
	glStencilOpSeparate =
	    (PFNGLSTENCILOPSEPARATEPROC)_rb_gl_get_proc_address("glStencilOpSeparate");
	glStencilFuncSeparate =
	    (PFNGLSTENCILFUNCSEPARATEPROC)_rb_gl_get_proc_address("glStencilFuncSeparate");
	glStencilMaskSeparate =
	    (PFNGLSTENCILMASKSEPARATEPROC)_rb_gl_get_proc_address("glStencilMaskSeparate");
	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)_rb_gl_get_proc_address("glDrawBuffers");

	glAttachShader = (PFNGLATTACHSHADERPROC)_rb_gl_get_proc_address("glAttachShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)_rb_gl_get_proc_address("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)_rb_gl_get_proc_address("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC)_rb_gl_get_proc_address("glCreateShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)_rb_gl_get_proc_address("glDeleteProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)_rb_gl_get_proc_address("glDeleteShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)_rb_gl_get_proc_address("glDetachShader");
	glGetAttachedShaders =
	    (PFNGLGETATTACHEDSHADERSPROC)_rb_gl_get_proc_address("glGetAttachedShaders");

	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)_rb_gl_get_proc_address("glGetProgramiv");
	glGetProgramInfoLog =
	    (PFNGLGETPROGRAMINFOLOGPROC)_rb_gl_get_proc_address("glGetProgramInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)_rb_gl_get_proc_address("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)_rb_gl_get_proc_address("glGetShaderInfoLog");
	glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)_rb_gl_get_proc_address("glGetShaderSource");
	glIsProgram = (PFNGLISPROGRAMPROC)_rb_gl_get_proc_address("glIsProgram");
	glIsShader = (PFNGLISSHADERPROC)_rb_gl_get_proc_address("glIsShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)_rb_gl_get_proc_address("glLinkProgram");
	glShaderSource = (PFNGLSHADERSOURCEPROC)_rb_gl_get_proc_address("glShaderSource");
	glUseProgram = (PFNGLUSEPROGRAMPROC)_rb_gl_get_proc_address("glUseProgram");
	glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)_rb_gl_get_proc_address("glValidateProgram");

	glUniform1i = (PFNGLUNIFORM1IPROC)_rb_gl_get_proc_address("glUniform1i");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)_rb_gl_get_proc_address("glUniform4fv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC)_rb_gl_get_proc_address("glUniform4iv");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)_rb_gl_get_proc_address("glUniformMatrix4fv");
	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)_rb_gl_get_proc_address("glGetActiveUniform");
	glGetUniformLocation =
	    (PFNGLGETUNIFORMLOCATIONPROC)_rb_gl_get_proc_address("glGetUniformLocation");
	glGetUniformfv = (PFNGLGETUNIFORMFVPROC)_rb_gl_get_proc_address("glGetUniformfv");
	glGetUniformiv = (PFNGLGETUNIFORMIVPROC)_rb_gl_get_proc_address("glGetUniformiv");

	glBindAttribLocation =
	    (PFNGLBINDATTRIBLOCATIONPROC)_rb_gl_get_proc_address("glBindAttribLocation");
	glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)_rb_gl_get_proc_address("glGetActiveAttrib");
	glGetAttribLocation =
	    (PFNGLGETATTRIBLOCATIONPROC)_rb_gl_get_proc_address("glGetAttribLocation");

	glDisableVertexAttribArray =
	    (PFNGLDISABLEVERTEXATTRIBARRAYPROC)_rb_gl_get_proc_address("glDisableVertexAttribArray");
	glEnableVertexAttribArray =
	    (PFNGLENABLEVERTEXATTRIBARRAYPROC)_rb_gl_get_proc_address("glEnableVertexAttribArray");
	glGetVertexAttribdv =
	    (PFNGLGETVERTEXATTRIBDVPROC)_rb_gl_get_proc_address("glGetVertexAttribdv");
	glGetVertexAttribfv =
	    (PFNGLGETVERTEXATTRIBFVPROC)_rb_gl_get_proc_address("glGetVertexAttribfv");
	glGetVertexAttribiv =
	    (PFNGLGETVERTEXATTRIBIVPROC)_rb_gl_get_proc_address("glGetVertexAttribiv");
	glGetVertexAttribPointerv =
	    (PFNGLGETVERTEXATTRIBPOINTERVPROC)_rb_gl_get_proc_address("glGetVertexAttribPointerv");
	glVertexAttribPointer =
	    (PFNGLVERTEXATTRIBPOINTERPROC)_rb_gl_get_proc_address("glVertexAttribPointer");

	if (!glBlendEquationSeparate || !glDrawBuffers || !glStencilOpSeparate ||
	    !glStencilFuncSeparate || !glStencilMaskSeparate || !glAttachShader || !glCompileShader ||
	    !glCreateProgram || !glCreateShader || !glDeleteProgram || !glDeleteShader ||
	    !glDetachShader || !glGetAttachedShaders || !glGetProgramiv || !glGetProgramInfoLog ||
	    !glGetShaderiv || !glGetShaderInfoLog || !glGetShaderSource || !glIsProgram ||
	    !glIsShader || !glLinkProgram || !glShaderSource || !glUseProgram || !glValidateProgram ||
	    !glUniform1i || !glUniform4fv || !glUniform4iv || !glUniformMatrix4fv ||
	    !glGetActiveUniform || !glGetUniformLocation || !glGetUniformfv || !glGetUniformiv ||
	    !glBindAttribLocation || !glGetActiveAttrib || !glGetAttribLocation ||
	    !glDisableVertexAttribArray || !glEnableVertexAttribArray || !glGetVertexAttribdv ||
	    !glGetVertexAttribfv || !glGetVertexAttribiv || !glGetVertexAttribPointerv ||
	    !glVertexAttribPointer) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED,
		          STRING_CONST("Unable to get GL procs for shaders"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_framebuffer_procs(void) {
	// We require GL_ARB_framebuffer_object extension
#ifndef GL_GLEXT_PROTOTYPES
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)_rb_gl_get_proc_address("glBindFramebuffer");
	glDeleteFramebuffers =
	    (PFNGLDELETEFRAMEBUFFERSPROC)_rb_gl_get_proc_address("glDeleteFramebuffers");
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)_rb_gl_get_proc_address("glGenFramebuffers");
	glCheckFramebufferStatus =
	    (PFNGLCHECKFRAMEBUFFERSTATUSPROC)_rb_gl_get_proc_address("glCheckFramebufferStatus");
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)_rb_gl_get_proc_address("glBindRenderbuffer");
	glDeleteRenderbuffers =
	    (PFNGLDELETERENDERBUFFERSPROC)_rb_gl_get_proc_address("glDeleteRenderbuffers");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)_rb_gl_get_proc_address("glGenRenderbuffers");
	glRenderbufferStorage =
	    (PFNGLRENDERBUFFERSTORAGEPROC)_rb_gl_get_proc_address("glRenderbufferStorage");
	glFramebufferTexture =
	    (PFNGLFRAMEBUFFERTEXTUREPROC)_rb_gl_get_proc_address("glFramebufferTexture");
	glFramebufferRenderbuffer =
	    (PFNGLFRAMEBUFFERRENDERBUFFERPROC)_rb_gl_get_proc_address("glFramebufferRenderbuffer");
	if (!glBindFramebuffer || !glDeleteFramebuffers || !glGenFramebuffers ||
	    !glCheckFramebufferStatus || !glBindRenderbuffer || !glDeleteRenderbuffers ||
	    !glGenRenderbuffers || !glRenderbufferStorage || !glFramebufferTexture ||
	    !glFramebufferRenderbuffer) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED,
		          STRING_CONST("Unable to get GL procs for frame buffers"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_arrays_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)_rb_gl_get_proc_address("glBindVertexArray");
	glDeleteVertexArrays =
	    (PFNGLDELETEVERTEXARRAYSPROC)_rb_gl_get_proc_address("glDeleteVertexArrays");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)_rb_gl_get_proc_address("glGenVertexArrays");
	if (!glBindVertexArray || !glDeleteVertexArrays || !glGenVertexArrays) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED,
		          STRING_CONST("Unable to get GL procs for vertex arrays"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_standard_procs(unsigned int major, unsigned int minor) {
	if ((major > 1) || ((major == 1) && (minor >= 4))) {
		if (!_rb_gl_get_texture_procs())
			return false;
		if (!_rb_gl_get_query_procs())
			return false;
		if (!_rb_gl_get_buffer_procs())
			return false;
	}
	if (major >= 2) {
		if (!_rb_gl_get_shader_procs())
			return false;
		if (!_rb_gl_get_framebuffer_procs())
			return false;
	}
	if (major >= 4) {
		if (!_rb_gl_get_arrays_procs())
			return false;
	}
	return true;
}

#endif
