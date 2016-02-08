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
#include <resource/types.h>

#include <render/build.h>

typedef enum render_api_t {
	RENDERAPI_UNKNOWN = 0,
	RENDERAPI_DEFAULT,
	RENDERAPI_NULL,
	RENDERAPI_OPENGL,
	RENDERAPI_OPENGL2,
	RENDERAPI_OPENGL3,
	RENDERAPI_OPENGL4,
	RENDERAPI_DIRECTX,
	RENDERAPI_DIRECTX10,
	RENDERAPI_DIRECTX11,
	RENDERAPI_PS3,
	RENDERAPI_PS4,
	RENDERAPI_XBOX360,
	RENDERAPI_XBOXONE,
	RENDERAPI_GLES,
	RENDERAPI_GLES2,
	RENDERAPI_GLES3,

	RENDERAPI_NUM
} render_api_t;

typedef enum render_api_group_t {
	RENDERAPIGROUP_NONE = 0,
	RENDERAPIGROUP_OPENGL,
	RENDERAPIGROUP_DIRECTX,
	RENDERAPIGROUP_GLES,

	RENDERAPIGROUP_NUM
} render_api_group_t;

typedef enum render_drawable_type_t {
	RENDERDRAWABLE_INVALID = 0,
	RENDERDRAWABLE_WINDOW,
	RENDERDRAWABLE_OFFSCREEN,
	RENDERDRAWABLE_FULLSCREEN
} render_drawable_type_t;

typedef enum render_usage_t {
	RENDERUSAGE_INVALID = 0,
	RENDERUSAGE_NORENDER,
	RENDERUSAGE_DYNAMIC,
	RENDERUSAGE_STATIC,
	RENDERUSAGE_TARGET
} render_usage_t;

typedef enum render_buffer_type_t {
	RENDERBUFFER_COLOR     = 0x01,
	RENDERBUFFER_DEPTH     = 0x02,
	RENDERBUFFER_STENCIL   = 0x04,

	RENDERBUFFER_VERTEX    = 0x10,
	RENDERBUFFER_INDEX     = 0x20,
	RENDERBUFFER_PARAMETER = 0x40
} render_buffer_type_t;

typedef enum render_buffer_uploadpolicy_t {
	RENDERBUFFER_UPLOAD_ONUNLOCK = 0,
	RENDERBUFFER_UPLOAD_ONRENDER,
	RENDERBUFFER_UPLOAD_ONDISPATCH
} render_buffer_uploadpolicy_t;

typedef enum render_buffer_flag_t {
	RENDERBUFFER_DIRTY            = 0x00000001,
	RENDERBUFFER_LOST             = 0x00000002,

	RENDERBUFFER_LOCK_READ        = 0x00010000,
	RENDERBUFFER_LOCK_WRITE       = 0x00020000,
	RENDERBUFFER_LOCK_NOUPLOAD    = 0x00040000,
	RENDERBUFFER_LOCK_FORCEUPLOAD = 0x00080000,
	RENDERBUFFER_LOCK_BITS        = 0x000F0000
} render_buffer_flag_t;

typedef enum render_vertex_format_t {
	//Never change order, code may rely on enum values being constant for optimization purposes
	VERTEXFORMAT_FLOAT = 0,
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

typedef enum render_vertex_attribute_id {
	VERTEXATTRIBUTE_POSITION = 0,
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

typedef enum render_shader_type_t {
	SHADER_COMPUTE  = 1,
	SHADER_GEOMETRY = 2,
	SHADER_VERTEX   = 4,
	SHADER_PIXEL    = 8
} render_shader_type_t;

typedef enum render_parameter_type_t {
	RENDERPARAMETER_FLOAT4 = 0,
	RENDERPARAMETER_INT4,
	RENDERPARAMETER_MATRIX,
	RENDERPARAMETER_TEXTURE,
	RENDERPARAMETER_ATTRIBUTE
} render_parameter_type_t;

typedef enum render_texture_type_t {
	RENDERTEXTURE_1D,
	RENDERTEXTURE_2D,
	RENDERTEXTURE_3D,
	RENDERTEXTURE_CUBE
} render_texture_type_t;

typedef enum render_blend_factor_t {
	BLEND_ZERO = 0,
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

typedef enum render_blend_op_t {
	BLEND_OP_ADD = 0,
	BLEND_OP_SUBTRACT,
	BLEND_OP_REV_SUBTRACT,
	BLEND_OP_MIN,
	BLEND_OP_MAX
} render_blend_op_t;

typedef enum render_pixelformat_t {
	PIXELFORMAT_INVALID = 0,

	PIXELFORMAT_R8G8B8X8,
	PIXELFORMAT_R8G8B8A8,
	PIXELFORMAT_R8G8B8,

	PIXELFORMAT_R32G32B32A32F,

	PIXELFORMAT_A8,

	PIXELFORMAT_PVRTC_2,
	PIXELFORMAT_PVRTC_4,

	PIXELFORMAT_NUMFORMATS
} pixelformat_t;

typedef enum render_colorspace_t {
	COLORSPACE_INVALID = 0,

	COLORSPACE_LINEAR,
	COLORSPACE_sRGB,

	COLORSPACE_NUMSPACES
} colorspace_t;

typedef enum render_primitive_t {
	RENDERPRIMITIVE_TRIANGLELIST = 0,

	RENDERPRIMITIVE_NUMTYPES
} render_primitive_t;

typedef enum render_command_id {
	RENDERCOMMAND_INVALID = 0,
	RENDERCOMMAND_CLEAR,
	RENDERCOMMAND_VIEWPORT,
	RENDERCOMMAND_RENDER_TRIANGLELIST
} render_command_id;

typedef struct render_backend_vtable_t render_backend_vtable_t;
typedef struct render_backend_t render_backend_t;
typedef struct render_drawable_t render_drawable_t;
typedef struct render_target_t render_target_t;
typedef struct render_context_t render_context_t;
typedef struct render_command_clear_t render_command_clear_t;
typedef struct render_command_viewport_t render_command_viewport_t;
typedef struct render_command_render_t render_command_render_t;
typedef struct render_command_t render_command_t;
typedef struct render_vertex_attribute_t render_vertex_attribute_t;
typedef struct render_vertex_decl_t render_vertex_decl_t;
typedef struct render_buffer_t render_buffer_t;
typedef struct render_vertexbuffer_t render_vertexbuffer_t;
typedef struct render_indexbuffer_t render_indexbuffer_t;
typedef struct render_shader_t render_shader_t;
typedef struct render_vertexshader_t render_vertexshader_t;
typedef struct render_pixelshader_t render_pixelshader_t;
typedef struct render_program_t render_program_t;
typedef struct render_resolution_t render_resolution_t;
typedef struct render_vertex_decl_element_t render_vertex_decl_element_t;
typedef struct render_parameter_t render_parameter_t;
typedef struct render_parameter_decl_t render_parameter_decl_t;
typedef struct render_parameterbuffer_t render_parameterbuffer_t;
typedef struct render_config_t render_config_t;

typedef bool (* render_backend_construct_fn)(render_backend_t*);
typedef void (* render_backend_destruct_fn)(render_backend_t*);
typedef unsigned int* (* render_backend_enumerate_adapters_fn)(render_backend_t*);
typedef render_resolution_t* (* render_backend_enumerate_modes_fn)(render_backend_t*, unsigned int);
typedef void (* render_backend_enable_thread_fn)(render_backend_t*);
typedef void (* render_backend_disable_thread_fn)(render_backend_t*);
typedef bool (* render_backend_set_drawable_fn)(render_backend_t*, render_drawable_t*);
typedef void (* render_backend_dispatch_fn)(render_backend_t*, render_context_t**, size_t);
typedef void (* render_backend_flip_fn)(render_backend_t*);
typedef void* (* render_backend_allocate_buffer_fn)(render_backend_t*, render_buffer_t*);
typedef void (* render_backend_deallocate_buffer_fn)(render_backend_t*, render_buffer_t*, bool,
                                                     bool);
typedef bool (* render_backend_upload_buffer_fn)(render_backend_t*, render_buffer_t*);
typedef bool (* render_backend_upload_shader_fn)(render_backend_t*, render_shader_t*, const void*,
                                                 size_t);
typedef bool (* render_backend_upload_program_fn)(render_backend_t*, render_program_t*);
typedef void* (* render_backend_read_shader_fn)(render_backend_t*, render_shader_t*, size_t*);
typedef void (* render_backend_deallocate_shader_fn)(render_backend_t*, render_shader_t*);
typedef void (* render_backend_deallocate_program_fn)(render_backend_t*, render_program_t*);

struct render_config_t {
	int unused;
};

struct render_backend_vtable_t {
	render_backend_construct_fn           construct;
	render_backend_destruct_fn            destruct;
	render_backend_enumerate_adapters_fn  enumerate_adapters;
	render_backend_enumerate_modes_fn     enumerate_modes;
	render_backend_enable_thread_fn       enable_thread;
	render_backend_disable_thread_fn      disable_thread;
	render_backend_set_drawable_fn        set_drawable;
	render_backend_dispatch_fn            dispatch;
	render_backend_flip_fn                flip;
	render_backend_allocate_buffer_fn     allocate_buffer;
	render_backend_upload_buffer_fn       upload_buffer;
	render_backend_upload_shader_fn       upload_shader;
	render_backend_upload_program_fn      upload_program;
	render_backend_read_shader_fn         read_shader;
	render_backend_deallocate_buffer_fn   deallocate_buffer;
	render_backend_deallocate_shader_fn   deallocate_shader;
	render_backend_deallocate_program_fn  deallocate_program;
};

#define RENDER_DECLARE_BACKEND \
	render_api_t             api; \
	render_api_group_t       api_group; \
	render_backend_vtable_t  vtable; \
	render_drawable_t*       drawable; \
	pixelformat_t            pixelformat; \
	colorspace_t             colorspace; \
	object_t                 framebuffer; \
	uint64_t                 framecount; \
	uint64_t                 platform

struct render_backend_t {
	RENDER_DECLARE_BACKEND;
};

struct render_drawable_t {
	render_drawable_type_t type;
	unsigned int adapter;
	window_t*    window;
#if FOUNDATION_PLATFORM_WINDOWS
	void* hwnd;
	void* hdc;
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	void* native; //EGL_DISPMANX_WINDOW_T*
	void* display;
#elif FOUNDATION_PLATFORM_LINUX
	void* display;
	int   screen;
	int   drawable;
#elif FOUNDATION_PLATFORM_MACOSX
	void* view;
#elif FOUNDATION_PLATFORM_IOS
	void* view;
	void* drawable; //CAEAGLLayer*
#elif FOUNDATION_PLATFORM_ANDROID
	void* native;
	void* display;
#else
#  error Not implemented
#endif
	object_t buffer;
	int      width;
	int      height;
	int      refresh;
};

#define RENDER_DECLARE_OBJECT \
	RESOURCE_DECLARE_OBJECT; \
	render_backend_t* backend

struct render_target_t {
	RENDER_DECLARE_OBJECT;
	int            width;
	int            height;
	pixelformat_t  pixelformat;
	colorspace_t   colorspace;
};

struct render_context_t {
	atomic32_t        reserved;
	int32_t           allocated;

	object_t          target;

	atomic64_t        key;

	render_command_t* commands;
	uint64_t*         keys;
	radixsort_t*      sort;
	uint8_t           group;

	const radixsort_index_t* order;
};

struct render_command_clear_t {
	unsigned int buffer_mask;
	uint32_t     color;
	unsigned int color_mask;
	uint32_t     depth;
	uint32_t     stencil;
};

struct render_command_viewport_t {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	real     min_z;
	real     max_z;
};

struct render_command_render_t {
	render_vertexshader_t* vertexshader;
	render_pixelshader_t* pixelshader;
	object_t vertexbuffer;
	object_t indexbuffer;
	object_t parameterbuffer;
	uint64_t blend_state;
};

struct render_command_t {
	unsigned int type: 8;
	unsigned int reserved: 8;
	unsigned int count: 16;

	union {
		render_command_clear_t    clear;
		render_command_viewport_t viewport;
		render_command_render_t   render;
	} data;
};

struct render_vertex_attribute_t {
	uint8_t  format;
	uint8_t  binding;
	uint16_t offset;
};

struct render_vertex_decl_t {
	unsigned int num_attributes;
	unsigned int size;
	render_vertex_attribute_t attribute[VERTEXATTRIBUTE_NUMATTRIBUTES];
};

struct render_parameter_t {
	hash_t                  name;
	render_parameter_type_t type;
	uint16_t                dim;
	uint16_t                offset;
	unsigned int            stages;
	unsigned int            location;
};

struct render_parameter_decl_t {
	unsigned int num_parameters;
	unsigned int size;
	render_parameter_t parameters[];
};

#define RENDER_DECLARE_BUFFER \
	RENDER_DECLARE_OBJECT; \
	uint8_t    usage; \
	uint8_t    buffertype; \
	uint8_t    policy; \
	uint8_t    __unused_buffer_0; \
	atomic32_t locks; \
	size_t     allocated; \
	size_t     used; \
	size_t     size; \
	void*      store; \
	void*      access; \
	object_t   vram; \
	uintptr_t  backend_data[4]

#define RENDER_DECLARE_SHADER \
	render_backend_t* backend; \
	unsigned int shadertype:8; \
	unsigned int unused:24; \
	uintptr_t backend_data[4]

struct render_buffer_t {
	RENDER_DECLARE_BUFFER;
};

struct render_vertexbuffer_t {
	RENDER_DECLARE_BUFFER;
	render_vertex_decl_t decl;
};

struct render_indexbuffer_t {
	RENDER_DECLARE_BUFFER;
};

struct render_parameterbuffer_t {
	RENDER_DECLARE_BUFFER;
	render_parameter_decl_t decl;
};

struct render_shader_t {
	RENDER_DECLARE_SHADER;
};

struct render_vertexshader_t {
	RENDER_DECLARE_SHADER;
};

struct render_pixelshader_t {
	RENDER_DECLARE_SHADER;
};

struct render_program_t {
	render_backend_t* backend;
	render_vertexshader_t* vertexshader;
	render_pixelshader_t* pixelshader;
	void* __unused;
#if FOUNDATION_SIZE_POINTER == 4
	uint32_t __padding_ptr[4];
#endif
	uintptr_t backend_data[4];
#if FOUNDATION_SIZE_POINTER == 4
	uint32_t __padding_data[4];
#endif
	render_vertex_decl_t attributes;
	render_parameter_decl_t parameters;
};

struct render_resolution_t {
	unsigned int  id;
	int           width;
	int           height;
	pixelformat_t pixelformat;
	colorspace_t  colorspace;
	unsigned int  refresh;
};

struct render_vertex_decl_element_t {
	render_vertex_format_t     format;
	render_vertex_attribute_id attribute;
};

