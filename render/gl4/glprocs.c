/* glprocs.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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
rb_gl_get_proc_address(const char* name) {
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
PFNGLUNIFORM1FPROC glUniform1f;
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
rb_gl_get_texture_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)rb_gl_get_proc_address("glActiveTexture");
	glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)rb_gl_get_proc_address("glSampleCoverage");
	glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)rb_gl_get_proc_address("glCompressedTexImage3D");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)rb_gl_get_proc_address("glCompressedTexImage2D");
	glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)rb_gl_get_proc_address("glCompressedTexImage1D");
	glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)rb_gl_get_proc_address("glCompressedTexSubImage3D");
	glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)rb_gl_get_proc_address("glCompressedTexSubImage2D");
	glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)rb_gl_get_proc_address("glCompressedTexSubImage1D");
	glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)rb_gl_get_proc_address("glGetCompressedTexImage");
	if (!glActiveTexture || !glSampleCoverage || !glCompressedTexImage3D || !glCompressedTexImage2D ||
	    !glCompressedTexImage1D || !glCompressedTexSubImage3D || !glCompressedTexSubImage2D ||
	    !glCompressedTexSubImage1D || !glGetCompressedTexImage) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for textures"));
		return false;
	}
	glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)rb_gl_get_proc_address("glGenerateMipmap");
#endif
	return true;
}

bool
rb_gl_get_query_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glGenQueries = (PFNGLGENQUERIESPROC)rb_gl_get_proc_address("glGenQueries");
	glDeleteQueries = (PFNGLDELETEQUERIESPROC)rb_gl_get_proc_address("glDeleteQueries");
	glIsQuery = (PFNGLISQUERYPROC)rb_gl_get_proc_address("glIsQuery");
	glBeginQuery = (PFNGLBEGINQUERYPROC)rb_gl_get_proc_address("glBeginQuery");
	glEndQuery = (PFNGLENDQUERYPROC)rb_gl_get_proc_address("glEndQuery");
	glGetQueryiv = (PFNGLGETQUERYIVPROC)rb_gl_get_proc_address("glGetQueryiv");
	glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)rb_gl_get_proc_address("glGetQueryObjectiv");
	glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)rb_gl_get_proc_address("glGetQueryObjectuiv");
	if (!glGenQueries || !glDeleteQueries || !glIsQuery || !glBeginQuery || !glEndQuery || !glGetQueryiv ||
	    !glGetQueryObjectiv || !glGetQueryObjectuiv) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for queries"));
		return false;
	}
#endif
	return true;
}

bool
rb_gl_get_buffer_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glBindBuffer = (PFNGLBINDBUFFERPROC)rb_gl_get_proc_address("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)rb_gl_get_proc_address("glDeleteBuffers");
	glGenBuffers = (PFNGLGENBUFFERSPROC)rb_gl_get_proc_address("glGenBuffers");
	glIsBuffer = (PFNGLISBUFFERPROC)rb_gl_get_proc_address("glIsBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)rb_gl_get_proc_address("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)rb_gl_get_proc_address("glBufferSubData");
	glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)rb_gl_get_proc_address("glGetBufferSubData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)rb_gl_get_proc_address("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)rb_gl_get_proc_address("glUnmapBuffer");
	glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)rb_gl_get_proc_address("glGetBufferParameteriv");
	glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)rb_gl_get_proc_address("glGetBufferPointerv");
	if (!glBindBuffer || !glDeleteBuffers || !glGenBuffers || !glIsBuffer || !glBufferData || !glBufferSubData ||
	    !glGetBufferSubData || !glMapBuffer || !glUnmapBuffer || !glGetBufferParameteriv || !glGetBufferPointerv) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for buffers"));
		return false;
	}
#endif
	return true;
}

bool
rb_gl_get_shader_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)rb_gl_get_proc_address("glBlendEquationSeparate");
	glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)rb_gl_get_proc_address("glStencilOpSeparate");
	glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)rb_gl_get_proc_address("glStencilFuncSeparate");
	glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)rb_gl_get_proc_address("glStencilMaskSeparate");
	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)rb_gl_get_proc_address("glDrawBuffers");

	glAttachShader = (PFNGLATTACHSHADERPROC)rb_gl_get_proc_address("glAttachShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)rb_gl_get_proc_address("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)rb_gl_get_proc_address("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC)rb_gl_get_proc_address("glCreateShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)rb_gl_get_proc_address("glDeleteProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)rb_gl_get_proc_address("glDeleteShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)rb_gl_get_proc_address("glDetachShader");
	glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)rb_gl_get_proc_address("glGetAttachedShaders");

	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)rb_gl_get_proc_address("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)rb_gl_get_proc_address("glGetProgramInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)rb_gl_get_proc_address("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)rb_gl_get_proc_address("glGetShaderInfoLog");
	glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)rb_gl_get_proc_address("glGetShaderSource");
	glIsProgram = (PFNGLISPROGRAMPROC)rb_gl_get_proc_address("glIsProgram");
	glIsShader = (PFNGLISSHADERPROC)rb_gl_get_proc_address("glIsShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)rb_gl_get_proc_address("glLinkProgram");
	glShaderSource = (PFNGLSHADERSOURCEPROC)rb_gl_get_proc_address("glShaderSource");
	glUseProgram = (PFNGLUSEPROGRAMPROC)rb_gl_get_proc_address("glUseProgram");
	glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)rb_gl_get_proc_address("glValidateProgram");

	glUniform1i = (PFNGLUNIFORM1IPROC)rb_gl_get_proc_address("glUniform1i");
	glUniform1f = (PFNGLUNIFORM1FPROC)rb_gl_get_proc_address("glUniform1f");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)rb_gl_get_proc_address("glUniform4fv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC)rb_gl_get_proc_address("glUniform4iv");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)rb_gl_get_proc_address("glUniformMatrix4fv");
	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)rb_gl_get_proc_address("glGetActiveUniform");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)rb_gl_get_proc_address("glGetUniformLocation");
	glGetUniformfv = (PFNGLGETUNIFORMFVPROC)rb_gl_get_proc_address("glGetUniformfv");
	glGetUniformiv = (PFNGLGETUNIFORMIVPROC)rb_gl_get_proc_address("glGetUniformiv");

	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)rb_gl_get_proc_address("glBindAttribLocation");
	glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)rb_gl_get_proc_address("glGetActiveAttrib");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)rb_gl_get_proc_address("glGetAttribLocation");

	glDisableVertexAttribArray =
	    (PFNGLDISABLEVERTEXATTRIBARRAYPROC)rb_gl_get_proc_address("glDisableVertexAttribArray");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)rb_gl_get_proc_address("glEnableVertexAttribArray");
	glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)rb_gl_get_proc_address("glGetVertexAttribdv");
	glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)rb_gl_get_proc_address("glGetVertexAttribfv");
	glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)rb_gl_get_proc_address("glGetVertexAttribiv");
	glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)rb_gl_get_proc_address("glGetVertexAttribPointerv");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)rb_gl_get_proc_address("glVertexAttribPointer");

	if (!glBlendEquationSeparate || !glDrawBuffers || !glStencilOpSeparate || !glStencilFuncSeparate ||
	    !glStencilMaskSeparate || !glAttachShader || !glCompileShader || !glCreateProgram || !glCreateShader ||
	    !glDeleteProgram || !glDeleteShader || !glDetachShader || !glGetAttachedShaders || !glGetProgramiv ||
	    !glGetProgramInfoLog || !glGetShaderiv || !glGetShaderInfoLog || !glGetShaderSource || !glIsProgram ||
	    !glIsShader || !glLinkProgram || !glShaderSource || !glUseProgram || !glValidateProgram || !glUniform1i ||
	    !glUniform4fv || !glUniform4iv || !glUniformMatrix4fv || !glGetActiveUniform || !glGetUniformLocation ||
	    !glGetUniformfv || !glGetUniformiv || !glBindAttribLocation || !glGetActiveAttrib || !glGetAttribLocation ||
	    !glDisableVertexAttribArray || !glEnableVertexAttribArray || !glGetVertexAttribdv || !glGetVertexAttribfv ||
	    !glGetVertexAttribiv || !glGetVertexAttribPointerv || !glVertexAttribPointer) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for shaders"));
		return false;
	}
#endif
	return true;
}

bool
rb_gl_get_framebuffer_procs(void) {
	// We require GL_ARB_framebuffer_object extension
#ifndef GL_GLEXT_PROTOTYPES
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)rb_gl_get_proc_address("glBindFramebuffer");
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)rb_gl_get_proc_address("glDeleteFramebuffers");
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)rb_gl_get_proc_address("glGenFramebuffers");
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)rb_gl_get_proc_address("glCheckFramebufferStatus");
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)rb_gl_get_proc_address("glBindRenderbuffer");
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)rb_gl_get_proc_address("glDeleteRenderbuffers");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)rb_gl_get_proc_address("glGenRenderbuffers");
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)rb_gl_get_proc_address("glRenderbufferStorage");
	glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)rb_gl_get_proc_address("glFramebufferTexture");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)rb_gl_get_proc_address("glFramebufferRenderbuffer");
	if (!glBindFramebuffer || !glDeleteFramebuffers || !glGenFramebuffers || !glCheckFramebufferStatus ||
	    !glBindRenderbuffer || !glDeleteRenderbuffers || !glGenRenderbuffers || !glRenderbufferStorage ||
	    !glFramebufferTexture || !glFramebufferRenderbuffer) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for frame buffers"));
		return false;
	}
#endif
	return true;
}

bool
rb_gl_get_arrays_procs(void) {
#ifndef GL_GLEXT_PROTOTYPES
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)rb_gl_get_proc_address("glBindVertexArray");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)rb_gl_get_proc_address("glDeleteVertexArrays");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)rb_gl_get_proc_address("glGenVertexArrays");
	if (!glBindVertexArray || !glDeleteVertexArrays || !glGenVertexArrays) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for vertex arrays"));
		return false;
	}
#endif
	return true;
}

bool
rb_gl_get_standard_procs(unsigned int major, unsigned int minor) {
	if ((major > 1) || ((major == 1) && (minor >= 4))) {
		if (!rb_gl_get_texture_procs())
			return false;
		if (!rb_gl_get_query_procs())
			return false;
		if (!rb_gl_get_buffer_procs())
			return false;
	}
	if (major >= 2) {
		if (!rb_gl_get_shader_procs())
			return false;
		if (!rb_gl_get_framebuffer_procs())
			return false;
	}
	if (major >= 4) {
		if (!rb_gl_get_arrays_procs())
			return false;
	}
	return true;
}

#endif
