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

#if !FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_MACOSX

extern PFNGLACTIVETEXTUREPROC                glActiveTexture;
extern PFNGLSAMPLECOVERAGEPROC               glSampleCoverage;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC         glCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC         glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC         glCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC      glCompressedTexSubImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC      glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC      glCompressedTexSubImage1D;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC        glGetCompressedTexImage;

#endif

#if !FOUNDATION_PLATFORM_MACOSX

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

#endif

typedef void (*glfn)( void );

RENDER_EXTERN glfn  _rb_gl_get_proc_address( const char* name );

RENDER_EXTERN bool  _rb_gl_get_texture_procs( void );
RENDER_EXTERN bool  _rb_gl_get_query_procs( void );
RENDER_EXTERN bool  _rb_gl_get_buffer_procs( void );
RENDER_EXTERN bool  _rb_gl_get_shader_procs( void );

RENDER_EXTERN bool  _rb_gl_get_standard_procs( int major, int minor );

RENDER_EXTERN bool  _rb_gl_check_error( const char* message );
RENDER_EXTERN bool  _rb_gl_check_context( int major, int minor );
RENDER_EXTERN bool  _rb_gl_check_extension( const char* name );
RENDER_EXTERN void* _rb_gl_create_context( render_drawable_t* drawable, int major, int minor, void* share_context );
RENDER_EXTERN void  _rb_gl_destroy_context( render_drawable_t* drawable, void* context );
