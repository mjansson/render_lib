/* glprocs.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#ifndef GL_GLEXT_PROTOTYPES

extern PFNGLACTIVETEXTUREPROC                glActiveTexture;
extern PFNGLSAMPLECOVERAGEPROC               glSampleCoverage;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC         glCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC         glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC         glCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC      glCompressedTexSubImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC      glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC      glCompressedTexSubImage1D;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC        glGetCompressedTexImage;

extern PFNGLGENQUERIESPROC                   glGenQueries;
extern PFNGLDELETEQUERIESPROC                glDeleteQueries;
extern PFNGLISQUERYPROC                      glIsQuery;
extern PFNGLBEGINQUERYPROC                   glBeginQuery;
extern PFNGLENDQUERYPROC                     glEndQuery;
extern PFNGLGETQUERYIVPROC                   glGetQueryiv;
extern PFNGLGETQUERYOBJECTIVPROC             glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUIVPROC            glGetQueryObjectuiv;

extern PFNGLBINDBUFFERPROC                   glBindBuffer;
extern PFNGLDELETEBUFFERSPROC                glDeleteBuffers;
extern PFNGLGENBUFFERSPROC                   glGenBuffers;
extern PFNGLISBUFFERPROC                     glIsBuffer;
extern PFNGLBUFFERDATAPROC                   glBufferData;
extern PFNGLBUFFERSUBDATAPROC                glBufferSubData;
extern PFNGLGETBUFFERSUBDATAPROC             glGetBufferSubData;
extern PFNGLMAPBUFFERPROC                    glMapBuffer;
extern PFNGLUNMAPBUFFERPROC                  glUnmapBuffer;
extern PFNGLGETBUFFERPARAMETERIVPROC         glGetBufferParameteriv;
extern PFNGLGETBUFFERPOINTERVPROC            glGetBufferPointerv;

extern PFNGLBLENDEQUATIONSEPARATEPROC        glBlendEquationSeparate;
extern PFNGLSTENCILOPSEPARATEPROC            glStencilOpSeparate;
extern PFNGLSTENCILFUNCSEPARATEPROC          glStencilFuncSeparate;
extern PFNGLSTENCILMASKSEPARATEPROC          glStencilMaskSeparate;
extern PFNGLDRAWBUFFERSPROC                  glDrawBuffers;

extern PFNGLATTACHSHADERPROC                 glAttachShader;
extern PFNGLCOMPILESHADERPROC                glCompileShader;
extern PFNGLCREATEPROGRAMPROC                glCreateProgram;
extern PFNGLCREATESHADERPROC                 glCreateShader;
extern PFNGLDELETEPROGRAMPROC                glDeleteProgram;
extern PFNGLDELETESHADERPROC                 glDeleteShader;
extern PFNGLDETACHSHADERPROC                 glDetachShader;
extern PFNGLGETATTACHEDSHADERSPROC           glGetAttachedShaders;

extern PFNGLGETPROGRAMIVPROC                 glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC            glGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC                  glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC             glGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC              glGetShaderSource;
extern PFNGLISPROGRAMPROC                    glIsProgram;
extern PFNGLISSHADERPROC                     glIsShader;
extern PFNGLLINKPROGRAMPROC                  glLinkProgram;
extern PFNGLSHADERSOURCEPROC                 glShaderSource;
extern PFNGLUSEPROGRAMPROC                   glUseProgram;
extern PFNGLVALIDATEPROGRAMPROC              glValidateProgram;

extern PFNGLUNIFORM1IPROC                    glUniform1i;
extern PFNGLUNIFORM4FVPROC                   glUniform4fv;
extern PFNGLUNIFORM4IVPROC                   glUniform4iv;
extern PFNGLUNIFORMMATRIX4FVPROC             glUniformMatrix4fv;
extern PFNGLGETACTIVEUNIFORMPROC             glGetActiveUniform;
extern PFNGLGETUNIFORMLOCATIONPROC           glGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC                 glGetUniformfv;
extern PFNGLGETUNIFORMIVPROC                 glGetUniformiv;

extern PFNGLBINDATTRIBLOCATIONPROC           glBindAttribLocation;
extern PFNGLGETACTIVEATTRIBPROC              glGetActiveAttrib;
extern PFNGLGETATTRIBLOCATIONPROC            glGetAttribLocation;

extern PFNGLDISABLEVERTEXATTRIBARRAYPROC     glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC      glEnableVertexAttribArray;
extern PFNGLGETVERTEXATTRIBDVPROC            glGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC            glGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC            glGetVertexAttribiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC      glGetVertexAttribPointerv;
extern PFNGLVERTEXATTRIBPOINTERPROC          glVertexAttribPointer;

extern PFNGLBINDFRAMEBUFFERPROC              glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC           glDeleteFramebuffers;
extern PFNGLGENFRAMEBUFFERSPROC              glGenFramebuffers;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC       glCheckFramebufferStatus;
extern PFNGLBINDRENDERBUFFERPROC             glBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC          glDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC             glGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC          glRenderbufferStorage;
extern PFNGLFRAMEBUFFERTEXTUREPROC           glFramebufferTexture;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC      glFramebufferRenderbuffer;

extern PFNGLBINDVERTEXARRAYPROC              glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC           glDeleteVertexArrays;
extern PFNGLGENVERTEXARRAYSPROC              glGenVertexArrays;

#endif

typedef void (*glfn)(void);

RENDER_EXTERN glfn
_rb_gl_get_proc_address(const char* name);

RENDER_EXTERN bool
_rb_gl_get_texture_procs(void);

RENDER_EXTERN bool
_rb_gl_get_query_procs(void);

RENDER_EXTERN bool
_rb_gl_get_buffer_procs(void);

RENDER_EXTERN bool
_rb_gl_get_shader_procs(void);

RENDER_EXTERN bool
_rb_gl_get_framebuffer_procs(void);

RENDER_EXTERN bool
_rb_gl_get_arrays_procs(void);

RENDER_EXTERN bool
_rb_gl_get_standard_procs(unsigned int major, unsigned int minor);

RENDER_EXTERN bool
_rb_gl_check_error(const char* message);

RENDER_EXTERN bool
_rb_gl_check_context(unsigned int major, unsigned int minor);

RENDER_EXTERN bool
_rb_gl_check_extension(const char* name, size_t length);

RENDER_EXTERN void*
_rb_gl_create_context(const render_drawable_t* drawable, unsigned int major, unsigned int minor, void* share_context);

RENDER_EXTERN void
_rb_gl_destroy_context(const render_drawable_t* drawable, void* context);

RENDER_EXTERN const char*
_rb_gl_error_message(GLenum err);

RENDER_EXTERN size_t
_rb_gl_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity);

RENDER_EXTERN size_t
_rb_gl_enumerate_modes(render_backend_t* backend, unsigned int adapter,
                       render_resolution_t* store, size_t capacity);

RENDER_EXTERN bool
_rb_gl_allocate_target(render_backend_t* backend, render_target_t* target);

RENDER_EXTERN void
_rb_gl_deallocate_target(render_backend_t* backend, render_target_t* target);

RENDER_EXTERN bool
_rb_gl_activate_target(render_backend_t* backend, render_target_t* target);

RENDER_EXTERN bool
_rb_gl_upload_texture(render_backend_t* backend, render_texture_t* texture,
                      const void* buffer, size_t size);

RENDER_EXTERN void
_rb_gl_deallocate_texture(render_backend_t* backend, render_texture_t* texture);

RENDER_EXTERN void
_rb_gl_parameter_bind_texture(render_backend_t* backend, void* buffer, render_texture_t* texture);

RENDER_EXTERN void
_rb_gl_parameter_bind_target(render_backend_t* backend, void* buffer, render_target_t* target);

RENDER_EXTERN void*
_rb_gl_get_thread_context(void);

RENDER_EXTERN void
_rb_gl_set_thread_context(void* context);
