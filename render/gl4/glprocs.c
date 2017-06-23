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

#include <foundation/log.h>

#include <render/backend.h>
#include <render/hashstrings.h>

#include <window/window.h>

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOS || ( FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI )

#include <render/gl4/glwrap.h>
#include <render/gl4/glprocs.h>

#if RENDER_ENABLE_NVGLEXPERT
#  include <nvapi.h>
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
#  error Not implemented
#endif
}

#if !FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_MACOS

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

#if !FOUNDATION_PLATFORM_MACOS

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
#if !FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_MACOS
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)_rb_gl_get_proc_address("glActiveTexture");
	glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)_rb_gl_get_proc_address("glSampleCoverage");
	glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)_rb_gl_get_proc_address("glCompressedTexImage3D");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)_rb_gl_get_proc_address("glCompressedTexImage2D");
	glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)_rb_gl_get_proc_address("glCompressedTexImage1D");
	glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)
	                            _rb_gl_get_proc_address("glCompressedTexSubImage3D");
	glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)
	                            _rb_gl_get_proc_address("glCompressedTexSubImage2D");
	glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)
	                            _rb_gl_get_proc_address("glCompressedTexSubImage1D");
	glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)
	                          _rb_gl_get_proc_address("glGetCompressedTexImage");
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
#if !FOUNDATION_PLATFORM_MACOS
	glGenQueries = (PFNGLGENQUERIESPROC)_rb_gl_get_proc_address("glGenQueries");
	glDeleteQueries = (PFNGLDELETEQUERIESPROC)_rb_gl_get_proc_address("glDeleteQueries");
	glIsQuery = (PFNGLISQUERYPROC)_rb_gl_get_proc_address("glIsQuery");
	glBeginQuery = (PFNGLBEGINQUERYPROC)_rb_gl_get_proc_address("glBeginQuery");
	glEndQuery = (PFNGLENDQUERYPROC)_rb_gl_get_proc_address("glEndQuery");
	glGetQueryiv = (PFNGLGETQUERYIVPROC)_rb_gl_get_proc_address("glGetQueryiv");
	glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)_rb_gl_get_proc_address("glGetQueryObjectiv");
	glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)_rb_gl_get_proc_address("glGetQueryObjectuiv");
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
#if !FOUNDATION_PLATFORM_MACOS
	glBindBuffer = (PFNGLBINDBUFFERPROC)_rb_gl_get_proc_address("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)_rb_gl_get_proc_address("glDeleteBuffers");
	glGenBuffers = (PFNGLGENBUFFERSPROC)_rb_gl_get_proc_address("glGenBuffers");
	glIsBuffer = (PFNGLISBUFFERPROC)_rb_gl_get_proc_address("glIsBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)_rb_gl_get_proc_address("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)_rb_gl_get_proc_address("glBufferSubData");
	glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)_rb_gl_get_proc_address("glGetBufferSubData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)_rb_gl_get_proc_address("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)_rb_gl_get_proc_address("glUnmapBuffer");
	glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)_rb_gl_get_proc_address("glGetBufferParameteriv");
	glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)_rb_gl_get_proc_address("glGetBufferPointerv");
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
#if !FOUNDATION_PLATFORM_MACOS
	glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)
	                          _rb_gl_get_proc_address("glBlendEquationSeparate");
	glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)_rb_gl_get_proc_address("glStencilOpSeparate");
	glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)_rb_gl_get_proc_address("glStencilFuncSeparate");
	glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)_rb_gl_get_proc_address("glStencilMaskSeparate");
	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)_rb_gl_get_proc_address("glDrawBuffers");

	glAttachShader = (PFNGLATTACHSHADERPROC)_rb_gl_get_proc_address("glAttachShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)_rb_gl_get_proc_address("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)_rb_gl_get_proc_address("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC)_rb_gl_get_proc_address("glCreateShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)_rb_gl_get_proc_address("glDeleteProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)_rb_gl_get_proc_address("glDeleteShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)_rb_gl_get_proc_address("glDetachShader");
	glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)_rb_gl_get_proc_address("glGetAttachedShaders");

	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)_rb_gl_get_proc_address("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)_rb_gl_get_proc_address("glGetProgramInfoLog");
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
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)_rb_gl_get_proc_address("glGetUniformLocation");
	glGetUniformfv = (PFNGLGETUNIFORMFVPROC)_rb_gl_get_proc_address("glGetUniformfv");
	glGetUniformiv = (PFNGLGETUNIFORMIVPROC)_rb_gl_get_proc_address("glGetUniformiv");

	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)_rb_gl_get_proc_address("glBindAttribLocation");
	glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)_rb_gl_get_proc_address("glGetActiveAttrib");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)_rb_gl_get_proc_address("glGetAttribLocation");

	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)
	                             _rb_gl_get_proc_address("glDisableVertexAttribArray");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)
	                            _rb_gl_get_proc_address("glEnableVertexAttribArray");
	glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)_rb_gl_get_proc_address("glGetVertexAttribdv");
	glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)_rb_gl_get_proc_address("glGetVertexAttribfv");
	glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)_rb_gl_get_proc_address("glGetVertexAttribiv");
	glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)
	                            _rb_gl_get_proc_address("glGetVertexAttribPointerv");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)_rb_gl_get_proc_address("glVertexAttribPointer");

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
	}
	return true;
}

#endif
