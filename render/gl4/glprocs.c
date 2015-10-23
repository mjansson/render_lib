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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <render/backend.h>
#include <window/window.h>

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOSX || ( FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI )

#include <render/gl4/glwrap.h>
#include <render/gl4/glprocs.h>

#if FOUNDATION_ENABLE_NVGLEXPERT
#  include <nvapi.h>
#endif

glfn
_rb_gl_get_proc_address(const char* name) {
#if FOUNDATION_PLATFORM_WINDOWS
	return (glfn)wglGetProcAddress(name);
#elif FOUNDATION_PLATFORM_LINUX
	return (glfn)glXGetProcAddressARB((const GLubyte*)name);
#elif FOUNDATION_PLATFORM_MACOSX
	return (glfn)dlsym(RTLD_DEFAULT, name);
#else
#  error Not implemented
#endif
}

#if !FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_MACOSX

PFNGLACTIVETEXTUREPROC                glActiveTexture = 0;
PFNGLSAMPLECOVERAGEPROC               glSampleCoverage = 0;
PFNGLCOMPRESSEDTEXIMAGE3DPROC         glCompressedTexImage3D = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC         glCompressedTexImage2D = 0;
PFNGLCOMPRESSEDTEXIMAGE1DPROC         glCompressedTexImage1D = 0;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC      glCompressedTexSubImage3D = 0;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC      glCompressedTexSubImage2D = 0;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC      glCompressedTexSubImage1D = 0;
PFNGLGETCOMPRESSEDTEXIMAGEPROC        glGetCompressedTexImage = 0;

#endif

#if !FOUNDATION_PLATFORM_MACOSX

PFNGLGENQUERIESPROC                   glGenQueries = 0;
PFNGLDELETEQUERIESPROC                glDeleteQueries = 0;
PFNGLISQUERYPROC                      glIsQuery = 0;
PFNGLBEGINQUERYPROC                   glBeginQuery = 0;
PFNGLENDQUERYPROC                     glEndQuery = 0;
PFNGLGETQUERYIVPROC                   glGetQueryiv = 0;
PFNGLGETQUERYOBJECTIVPROC             glGetQueryObjectiv = 0;
PFNGLGETQUERYOBJECTUIVPROC            glGetQueryObjectuiv = 0;

PFNGLBINDBUFFERPROC                   glBindBuffer = 0;
PFNGLDELETEBUFFERSPROC                glDeleteBuffers = 0;
PFNGLGENBUFFERSPROC                   glGenBuffers = 0;
PFNGLISBUFFERPROC                     glIsBuffer = 0;
PFNGLBUFFERDATAPROC                   glBufferData = 0;
PFNGLBUFFERSUBDATAPROC                glBufferSubData = 0;
PFNGLGETBUFFERSUBDATAPROC             glGetBufferSubData = 0;
PFNGLMAPBUFFERPROC                    glMapBuffer = 0;
PFNGLUNMAPBUFFERPROC                  glUnmapBuffer = 0;
PFNGLGETBUFFERPARAMETERIVPROC         glGetBufferParameteriv = 0;
PFNGLGETBUFFERPOINTERVPROC            glGetBufferPointerv = 0;

PFNGLBLENDEQUATIONSEPARATEPROC        glBlendEquationSeparate = 0;
PFNGLDRAWBUFFERSPROC                  glDrawBuffers = 0;
PFNGLSTENCILOPSEPARATEPROC            glStencilOpSeparate = 0;
PFNGLSTENCILFUNCSEPARATEPROC          glStencilFuncSeparate = 0;
PFNGLSTENCILMASKSEPARATEPROC          glStencilMaskSeparate = 0;

PFNGLATTACHSHADERPROC                 glAttachShader = 0;
PFNGLCOMPILESHADERPROC                glCompileShader = 0;
PFNGLCREATEPROGRAMPROC                glCreateProgram = 0;
PFNGLCREATESHADERPROC                 glCreateShader = 0;
PFNGLDELETEPROGRAMPROC                glDeleteProgram = 0;
PFNGLDELETESHADERPROC                 glDeleteShader = 0;
PFNGLDETACHSHADERPROC                 glDetachShader = 0;
PFNGLGETATTACHEDSHADERSPROC           glGetAttachedShaders = 0;

PFNGLGETPROGRAMIVPROC                 glGetProgramiv = 0;
PFNGLGETPROGRAMINFOLOGPROC            glGetProgramInfoLog = 0;
PFNGLGETSHADERIVPROC                  glGetShaderiv = 0;
PFNGLGETSHADERINFOLOGPROC             glGetShaderInfoLog = 0;
PFNGLGETSHADERSOURCEPROC              glGetShaderSource = 0;
PFNGLISPROGRAMPROC                    glIsProgram = 0;
PFNGLISSHADERPROC                     glIsShader = 0;
PFNGLLINKPROGRAMPROC                  glLinkProgram = 0;
PFNGLSHADERSOURCEPROC                 glShaderSource = 0;
PFNGLUSEPROGRAMPROC                   glUseProgram = 0;
PFNGLVALIDATEPROGRAMPROC              glValidateProgram = 0;

PFNGLUNIFORM1IPROC                    glUniform1i = 0;
PFNGLUNIFORM4FVPROC                   glUniform4fv = 0;
PFNGLUNIFORM4IVPROC                   glUniform4iv = 0;
PFNGLUNIFORMMATRIX4FVPROC             glUniformMatrix4fv = 0;
PFNGLGETACTIVEUNIFORMPROC             glGetActiveUniform = 0;
PFNGLGETUNIFORMLOCATIONPROC           glGetUniformLocation = 0;
PFNGLGETUNIFORMFVPROC                 glGetUniformfv = 0;
PFNGLGETUNIFORMIVPROC                 glGetUniformiv = 0;

PFNGLBINDATTRIBLOCATIONPROC           glBindAttribLocation = 0;
PFNGLGETACTIVEATTRIBPROC              glGetActiveAttrib = 0;
PFNGLGETATTRIBLOCATIONPROC            glGetAttribLocation = 0;

PFNGLDISABLEVERTEXATTRIBARRAYPROC     glDisableVertexAttribArray = 0;
PFNGLENABLEVERTEXATTRIBARRAYPROC      glEnableVertexAttribArray = 0;
PFNGLGETVERTEXATTRIBDVPROC            glGetVertexAttribdv = 0;
PFNGLGETVERTEXATTRIBFVPROC            glGetVertexAttribfv = 0;
PFNGLGETVERTEXATTRIBIVPROC            glGetVertexAttribiv = 0;
PFNGLGETVERTEXATTRIBPOINTERVPROC      glGetVertexAttribPointerv = 0;
PFNGLVERTEXATTRIBPOINTERPROC          glVertexAttribPointer = 0;

#endif

bool
_rb_gl_get_texture_procs(void) {
#if !FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_MACOSX
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)glGetProcAddress("glActiveTexture");
	glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)glGetProcAddress("glSampleCoverage");
	glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)glGetProcAddress("glCompressedTexImage3D");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)glGetProcAddress("glCompressedTexImage2D");
	glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)glGetProcAddress("glCompressedTexImage1D");
	glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)
	                            glGetProcAddress("glCompressedTexSubImage3D");
	glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)
	                            glGetProcAddress("glCompressedTexSubImage2D");
	glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)
	                            glGetProcAddress("glCompressedTexSubImage1D");
	glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)
	                          glGetProcAddress("glGetCompressedTexImage");
	if (!glActiveTexture || !glSampleCoverage || !glCompressedTexImage3D || !glCompressedTexImage2D ||
	        !glCompressedTexImage1D || !glCompressedTexSubImage3D || !glCompressedTexSubImage2D ||
	        !glCompressedTexSubImage1D || !glGetCompressedTexImage) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for textures"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_query_procs(void) {
#if !FOUNDATION_PLATFORM_MACOSX
	glGenQueries = (PFNGLGENQUERIESPROC)glGetProcAddress("glGenQueries");
	glDeleteQueries = (PFNGLDELETEQUERIESPROC)glGetProcAddress("glDeleteQueries");
	glIsQuery = (PFNGLISQUERYPROC)glGetProcAddress("glIsQuery");
	glBeginQuery = (PFNGLBEGINQUERYPROC)glGetProcAddress("glBeginQuery");
	glEndQuery = (PFNGLENDQUERYPROC)glGetProcAddress("glEndQuery");
	glGetQueryiv = (PFNGLGETQUERYIVPROC)glGetProcAddress("glGetQueryiv");
	glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)glGetProcAddress("glGetQueryObjectiv");
	glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)glGetProcAddress("glGetQueryObjectuiv");
	if (!glGenQueries || !glDeleteQueries || !glIsQuery || !glBeginQuery || !glEndQuery ||
	        !glGetQueryiv || !glGetQueryObjectiv || !glGetQueryObjectuiv) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for queries"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_buffer_procs(void) {
#if !FOUNDATION_PLATFORM_MACOSX
	glBindBuffer = (PFNGLBINDBUFFERPROC)glGetProcAddress("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glGetProcAddress("glDeleteBuffers");
	glGenBuffers = (PFNGLGENBUFFERSPROC)glGetProcAddress("glGenBuffers");
	glIsBuffer = (PFNGLISBUFFERPROC)glGetProcAddress("glIsBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)glGetProcAddress("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)glGetProcAddress("glBufferSubData");
	glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)glGetProcAddress("glGetBufferSubData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)glGetProcAddress("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)glGetProcAddress("glUnmapBuffer");
	glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)glGetProcAddress("glGetBufferParameteriv");
	glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)glGetProcAddress("glGetBufferPointerv");
	if (!glBindBuffer || !glDeleteBuffers || !glGenBuffers || !glIsBuffer || !glBufferData ||
	        !glBufferSubData || !glGetBufferSubData || !glMapBuffer || !glUnmapBuffer ||
	        !glGetBufferParameteriv || !glGetBufferPointerv) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for buffers"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_shader_procs(void) {
#if !FOUNDATION_PLATFORM_MACOSX
	glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)
	                          glGetProcAddress("glBlendEquationSeparate");
	glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)glGetProcAddress("glStencilOpSeparate");
	glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)glGetProcAddress("glStencilFuncSeparate");
	glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)glGetProcAddress("glStencilMaskSeparate");
	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)glGetProcAddress("glDrawBuffers");

	glAttachShader = (PFNGLATTACHSHADERPROC)glGetProcAddress("glAttachShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)glGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)glGetProcAddress("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC)glGetProcAddress("glCreateShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glGetProcAddress("glDeleteProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)glGetProcAddress("glDeleteShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)glGetProcAddress("glDetachShader");
	glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)glGetProcAddress("glGetAttachedShaders");

	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glGetProcAddress("glGetProgramInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)glGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glGetProcAddress("glGetShaderInfoLog");
	glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)glGetProcAddress("glGetShaderSource");
	glIsProgram = (PFNGLISPROGRAMPROC)glGetProcAddress("glIsProgram");
	glIsShader = (PFNGLISSHADERPROC)glGetProcAddress("glIsShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)glGetProcAddress("glLinkProgram");
	glShaderSource = (PFNGLSHADERSOURCEPROC)glGetProcAddress("glShaderSource");
	glUseProgram = (PFNGLUSEPROGRAMPROC)glGetProcAddress("glUseProgram");
	glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)glGetProcAddress("glValidateProgram");

	glUniform1i = (PFNGLUNIFORM1IPROC)glGetProcAddress("glUniform1i");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)glGetProcAddress("glUniform4fv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC)glGetProcAddress("glUniform4iv");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)glGetProcAddress("glUniformMatrix4fv");
	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)glGetProcAddress("glGetActiveUniform");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glGetProcAddress("glGetUniformLocation");
	glGetUniformfv = (PFNGLGETUNIFORMFVPROC)glGetProcAddress("glGetUniformfv");
	glGetUniformiv = (PFNGLGETUNIFORMIVPROC)glGetProcAddress("glGetUniformiv");

	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)glGetProcAddress("glBindAttribLocation");
	glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)glGetProcAddress("glGetActiveAttrib");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)glGetProcAddress("glGetAttribLocation");

	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)
	                             glGetProcAddress("glDisableVertexAttribArray");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)
	                            glGetProcAddress("glEnableVertexAttribArray");
	glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)glGetProcAddress("glGetVertexAttribdv");
	glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)glGetProcAddress("glGetVertexAttribfv");
	glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)glGetProcAddress("glGetVertexAttribiv");
	glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)
	                            glGetProcAddress("glGetVertexAttribPointerv");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glGetProcAddress("glVertexAttribPointer");

	if (!glBlendEquationSeparate || !glDrawBuffers || !glStencilOpSeparate || !glStencilFuncSeparate ||
	        !glStencilMaskSeparate || !glAttachShader || !glCompileShader || !glCreateProgram ||
	        !glCreateShader || !glDeleteProgram || !glDeleteShader || !glDetachShader ||
	        !glGetAttachedShaders || !glGetProgramiv || !glGetProgramInfoLog || !glGetShaderiv ||
	        !glGetShaderInfoLog || !glGetShaderSource || !glIsProgram || !glIsShader || !glLinkProgram ||
	        !glShaderSource || !glUseProgram || !glValidateProgram || !glUniform1i || !glUniform4fv ||
	        !glUniform4iv || !glUniformMatrix4fv || !glGetActiveUniform || !glGetUniformLocation ||
	        !glGetUniformfv || !glGetUniformiv || !glBindAttribLocation || !glGetActiveAttrib ||
	        !glGetAttribLocation || !glDisableVertexAttribArray || !glEnableVertexAttribArray ||
	        !glGetVertexAttribdv || !glGetVertexAttribfv || !glGetVertexAttribiv ||
	        !glGetVertexAttribPointerv || !glVertexAttribPointer) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to get GL procs for shaders"));
		return false;
	}
#endif
	return true;
}

bool
_rb_gl_get_standard_procs(int major, int minor) {
	if ((major > 1) || ((major == 1) && (minor >= 4))) {
		if (!_rb_gl_get_texture_procs())
			return false;
		if (!_rb_gl_get_query_procs())
			return false;
		if (!_rb_gl_get_buffer_procs())
			return false;
	}
	if ((major > 2) || ((major == 2) && (minor >= 0))) {
		if (!_rb_gl_get_shader_procs())
			return false;
	}
	return true;
}

#endif
