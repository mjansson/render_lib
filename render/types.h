/* types.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file types.h
    Render data types */

#include <foundation/platform.h>
#include <foundation/types.h>

#include <window/types.h>

#include <render/build.h>


//CONSTANTS

typedef enum _render_api
{
	RENDERAPI_UNKNOWN                       = 0,
	RENDERAPI_DEFAULT,
	RENDERAPI_NULL,
	RENDERAPI_OPENGL2,
	RENDERAPI_OPENGL3,
	RENDERAPI_OPENGL4,
	RENDERAPI_DIRECTX10,
	RENDERAPI_DIRECTX11,
	RENDERAPI_PS3,
	RENDERAPI_PS4,
	RENDERAPI_XBOX360,
	RENDERAPI_XBOXONE,
	RENDERAPI_GLES1,
	RENDERAPI_GLES2,
    
	RENDERAPI_NUM
} render_api_t;

typedef enum _render_drawable_type
{
	RENDERDRAWABLE_INVALID                 = 0,
	RENDERDRAWABLE_WINDOW,
	RENDERDRAWABLE_OFFSCREEN,
	RENDERDRAWABLE_FULLSCREEN
} render_drawable_type_t;

typedef enum _render_usage
{
	RENDERUSAGE_INVALID                    = 0,
	RENDERUSAGE_NORENDER,
	RENDERUSAGE_DYNAMIC,
	RENDERUSAGE_STATIC,
	RENDERUSAGE_TARGET
} render_usage_t;

typedef enum _render_buffer_type
{
	RENDERBUFFER_COLOR                     = 0x01,
	RENDERBUFFER_DEPTH                     = 0x02,
	RENDERBUFFER_STENCIL                   = 0x04,
    
	RENDERBUFFER_VERTEX                    = 0x10,
	RENDERBUFFER_INDEX                     = 0x20
} render_buffer_type_t;

typedef enum _render_buffer_uploadpolicy
{
	RENDERBUFFER_UPLOAD_ONUNLOCK           = 0,
	RENDERBUFFER_UPLOAD_ONRENDER,
	RENDERBUFFER_UPLOAD_ONDISPATCH
} render_buffer_uploadpolicy_t;

typedef enum _render_buffer_flag
{
	RENDERBUFFER_DIRTY                     = 0x00000001,
	RENDERBUFFER_LOST                      = 0x00000002,
    
	RENDERBUFFER_LOCK_READ                 = 0x00010000,
	RENDERBUFFER_LOCK_WRITE                = 0x00020000,
	RENDERBUFFER_LOCK_NOUPLOAD             = 0x00040000,
	RENDERBUFFER_LOCK_FORCEUPLOAD          = 0x00080000,
	RENDERBUFFER_LOCK_BITS                 = 0x000F0000
} render_buffer_flag_t;

typedef enum _render_vertex_format
{
	//Never change order, code may rely on enum values being constant for optimization purposes
	VERTEXFORMAT_FLOAT                     = 0,
	VERTEXFORMAT_FLOAT2,
	VERTEXFORMAT_FLOAT3,
	VERTEXFORMAT_FLOAT4,
    
	//! Unsigned normalized
	VERTEXFORMAT_UBYTE4_UNORM,
	//! Signed normalized
	VERTEXFORMAT_UBYTE4_SNORM,
    
	VERTEXFORMAT_SHORT,
	VERTEXFORMAT_SHORT2,
	VERTEXFORMAT_SHORT4,
    
	VERTEXFORMAT_INT,
	VERTEXFORMAT_INT2,
	VERTEXFORMAT_INT4,
    
	VERTEXFORMAT_NUMTYPES,
	VERTEXFORMAT_UNKNOWN
} render_vertex_format_t;

typedef enum _render_vertex_attribute_id
{
	VERTEXATTRIBUTE_POSITION                = 0,
	VERTEXATTRIBUTE_WEIGHT,
	VERTEXATTRIBUTE_NORMAL,
	VERTEXATTRIBUTE_PRIMARYCOLOR,
	VERTEXATTRIBUTE_SECONDARYCOLOR,
	VERTEXATTRIBUTE_UNNAMED_05,
	VERTEXATTRIBUTE_UNNAMED_06,
	VERTEXATTRIBUTE_INDEX,
	VERTEXATTRIBUTE_TEXCOORD0,
	VERTEXATTRIBUTE_TEXCOORD1,
	VERTEXATTRIBUTE_TEXCOORD2,
	VERTEXATTRIBUTE_TEXCOORD3,
	VERTEXATTRIBUTE_TEXCOORD4,
	VERTEXATTRIBUTE_TEXCOORD5,
	VERTEXATTRIBUTE_TANGENT,
	VERTEXATTRIBUTE_BINORMAL,
    
	VERTEXATTRIBUTE_NUMATTRIBUTES
} render_vertex_attribute_id;

typedef enum _render_shader_type
{
	SHADER_COMPUTE                          = 1,
	SHADER_GEOMETRY                         = 2,
	SHADER_VERTEX                           = 4,
	SHADER_PIXEL                            = 8
} render_shader_type_t;

typedef enum _render_parameter_type
{
	RENDERPARAMETER_FLOAT4                  = 0,
	RENDERPARAMETER_INT4,
	RENDERPARAMETER_MATRIX,
	RENDERPARAMETER_TEXTURE
	//If adding new, increase storage size in render_parameter_info_t
} render_parameter_type_t;

typedef enum _render_texture_type
{
	RENDERTEXTURE_1D,
	RENDERTEXTURE_2D,
	RENDERTEXTURE_3D,
	RENDERTEXTURE_CUBE
} render_texture_type_t;

typedef enum _render_blend_factor
{
	BLEND_ZERO                              = 0,
	BLEND_ONE,
	BLEND_SRCCOLOR,
	BLEND_INVSRCCOLOR,
	BLEND_DESTCOLOR,
	BLEND_INVDESTCOLOR,
	BLEND_SRCALPHA,
	BLEND_INVSRCALPHA,
	BLEND_DESTALPHA,
	BLEND_INVDESTALPHA,
	BLEND_FACTOR,
	BLEND_INVFACTOR,
	BLEND_SRCALPHASAT
} render_blend_factor_t;

typedef enum _render_blend_op
{
	BLEND_OP_ADD            = 0,
	BLEND_OP_SUBTRACT,
	BLEND_OP_REV_SUBTRACT,
	BLEND_OP_MIN,
	BLEND_OP_MAX
} render_blend_op_t;

typedef enum _render_pixelformat
{
	PIXELFORMAT_INVALID                     = 0,
    
	PIXELFORMAT_R8G8B8X8,
	PIXELFORMAT_R8G8B8A8,
	PIXELFORMAT_R8G8B8,
    
	PIXELFORMAT_R32G32B32A32F,
    
	PIXELFORMAT_A8,
    
	PIXELFORMAT_PVRTC_2,
	PIXELFORMAT_PVRTC_4,
    
	PIXELFORMAT_NUMFORMATS
} pixelformat_t;

typedef enum _render_colorspace
{
	COLORSPACE_INVALID                      = 0,
	
	COLORSPACE_LINEAR,
	COLORSPACE_sRGB,
    
	COLORSPACE_NUMSPACES
} colorspace_t;

typedef enum _render_primitive
{
	RENDERPRIMITIVE_TRIANGLELIST            = 0,
	
	RENDERPRIMITIVE_NUMTYPES
} render_primitive_t;

// OPAQUE COMPLEX TYPES

typedef struct render_backend_t        render_backend_t;
typedef struct render_target_t         render_target_t;
typedef struct render_context_t        render_context_t;
typedef struct render_drawable_t       render_drawable_t;
typedef struct render_command_t        render_command_t;
typedef struct render_buffer_t         render_buffer_t;
typedef struct render_vertex_decl_t    render_vertex_decl_t;
typedef struct render_shader_t         render_shader_t;

// COMPLEX TYPES

typedef struct render_resolution_t
{
	unsigned int                      id;
	unsigned int                      width;
	unsigned int                      height;
	pixelformat_t                     pixelformat;
	colorspace_t                      colorspace;
	unsigned int                      refresh;
} render_resolution_t;

typedef struct render_vertex_decl_element_t
{
	render_vertex_format_t      format;
	render_vertex_attribute_id  attribute;
} render_vertex_decl_element_t;

// EXTERNAL OPAQUE TYPES
typedef struct window_t                window_t;

