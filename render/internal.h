/* internal.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

/*! \file internal.h
    Internal types */

#include <foundation/platform.h>
#include <foundation/types.h>
#include <foundation/internal.h>

#include <window/types.h>
#include <resource/types.h>

#include <render/types.h>
#include <render/hashstrings.h>


typedef bool                                   (* render_backend_construct_fn)( render_backend_t* );
typedef void                                   (* render_backend_destruct_fn)( render_backend_t* );
typedef unsigned int*                          (* render_backend_enumerate_adapters_fn)( render_backend_t* );
typedef render_resolution_t*                   (* render_backend_enumerate_modes_fn)( render_backend_t*, unsigned int );
typedef void                                   (* render_backend_enable_thread_fn)( render_backend_t* );
typedef void                                   (* render_backend_disable_thread_fn)( render_backend_t* );
typedef bool                                   (* render_backend_set_drawable_fn)( render_backend_t*, render_drawable_t* );
typedef void                                   (* render_backend_dispatch_fn)( render_backend_t*, render_context_t**, unsigned int );
typedef void                                   (* render_backend_flip_fn)( render_backend_t* );
typedef void*                                  (* render_backend_allocate_buffer_fn)( render_backend_t*, render_buffer_t* );
typedef void                                   (* render_backend_deallocate_buffer_fn)( render_backend_t*, render_buffer_t*, bool, bool );
typedef void                                   (* render_backend_upload_buffer_fn)( render_backend_t*, render_buffer_t* );
typedef void                                   (* render_backend_upload_shader_fn)( render_backend_t*, render_shader_t*, const void*, unsigned int );
typedef void*                                  (* render_backend_read_shader_fn)( render_backend_t*, render_shader_t*, uint64_t* );
typedef void                                   (* render_backend_deallocate_shader_fn)( render_backend_t*, render_shader_t* );

typedef struct render_backend_vtable_t
{
	render_backend_construct_fn                   construct;
	render_backend_destruct_fn                    destruct;
	render_backend_enumerate_adapters_fn          enumerate_adapters;
	render_backend_enumerate_modes_fn             enumerate_modes;
	render_backend_enable_thread_fn               enable_thread;
	render_backend_disable_thread_fn              disable_thread;
	render_backend_set_drawable_fn                set_drawable;
	render_backend_dispatch_fn                    dispatch;
	render_backend_flip_fn                        flip;
	render_backend_allocate_buffer_fn             allocate_buffer;
	render_backend_deallocate_buffer_fn           deallocate_buffer;
	render_backend_upload_buffer_fn               upload_buffer;
	render_backend_upload_shader_fn               upload_shader;
	render_backend_read_shader_fn                 read_shader;
	render_backend_deallocate_shader_fn           deallocate_shader;
} render_backend_vtable_t;

#define RENDER_DECLARE_BACKEND							\
	render_api_t                      api;				\
	render_backend_vtable_t           vtable;			\
	render_drawable_t*                drawable;			\
	pixelformat_t                     pixelformat;      \
	colorspace_t                      colorspace;       \
	object_t                          framebuffer;      \
	uint64_t                          framecount;

struct render_backend_t
{
	RENDER_DECLARE_BACKEND;
};


struct render_drawable_t
{
	render_drawable_type_t      type;
	unsigned int                adapter;
	window_t*                   window;
#if FOUNDATION_PLATFORM_WINDOWS
	void*                       hwnd;
	void*                       hdc;
#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	void*                       native; //EGL_DISPMANX_WINDOW_T*
	void*                       display;
#elif FOUNDATION_PLATFORM_LINUX
	void*                       display;
	int                         screen;
	int                         drawable;
#elif FOUNDATION_PLATFORM_MACOSX
	void*                       view;
#elif FOUNDATION_PLATFORM_IOS
	void*                       view;
	void*                       drawable; //CAEAGLLayer*
#elif FOUNDATION_PLATFORM_ANDROID
	void*                       native;
	void*                       display;
#else
#  error Not implemented
#endif
	object_t                    buffer;
	unsigned int                width;
	unsigned int                height;
	unsigned int                refresh;
};

#define RENDER_DECLARE_OBJECT             \
	RESOURCE_DECLARE_OBJECT;              \
	render_backend_t*           backend

struct render_target_t
{
	RENDER_DECLARE_OBJECT;
	unsigned int                width;
	unsigned int                height;
	pixelformat_t               pixelformat;
	colorspace_t                colorspace;
};

struct render_context_t
{
	atomic32_t                  reserved;
	int32_t                     allocated;
    
	object_t                    target;
    
	atomic64_t                  key;
    
	render_command_t*           commands;
	uint64_t*                   keys;
	radixsort_t*                sort;
	uint8_t                     group;
    
	const radixsort_index_t*    order;
};

typedef struct render_command_clear_t
{
	unsigned int                buffer_mask;
	uint32_t                    color;
	unsigned int                color_mask;
	uint32_t                    depth;
	uint32_t                    stencil;
} render_command_clear_t;

typedef struct render_command_viewport_t
{
	uint16_t                    x;
	uint16_t                    y;
	uint16_t                    width;
	uint16_t                    height;
	real                        min_z;
	real                        max_z;
} render_command_viewport_t;

typedef struct render_command_render_t
{
	object_t                    vertexshader;
	object_t                    pixelshader;
	object_t                    vertexbuffer;
	object_t                    indexbuffer;
	object_t                    parameterblock;
	uint64_t                    blend_state;
} render_command_render_t;

struct render_command_t
{
	unsigned int                  type:8;
	unsigned int                  reserved:8;
	unsigned int                  count:16;
	
	union
	{
		render_command_clear_t    clear;
		render_command_viewport_t viewport;
		render_command_render_t   render;
	} data;
};

typedef enum render_command_id
{
	RENDERCOMMAND_INVALID                   = 0,
	RENDERCOMMAND_CLEAR,
	RENDERCOMMAND_VIEWPORT,
	RENDERCOMMAND_RENDER_TRIANGLELIST
} render_command_id;

typedef struct render_vertex_attribute_t
{
	uint8_t                           format;
	uint8_t                           binding;
	uint16_t                          offset;
} render_vertex_attribute_t;

typedef struct render_vertex_decl_t
{
	render_vertex_attribute_t         attribute[VERTEXATTRIBUTE_NUMATTRIBUTES];
} render_vertex_decl_t;

#define RENDER_DECLARE_BUFFER \
	RENDER_DECLARE_OBJECT;                           \
	uint8_t                       usage;             \
	uint8_t                       buffertype;        \
	uint8_t                       policy;            \
	uint8_t                       __unused_buffer_0; \
	atomic32_t                    locks;             \
	unsigned int                  allocated;         \
	unsigned int                  used;              \
	unsigned int                  size;              \
	void*                         store;             \
	void*                         access;            \
	object_t                      vram;              \
	uintptr_t                     backend_data[4]

#define RENDER_DECLARE_SHADER \
	RENDER_DECLARE_OBJECT;                           \
	int32_t                       shadertype:8;      \
	int32_t                       unused:24;         \
	uintptr_t                     backend_data[4]

typedef struct render_buffer_t
{
	RENDER_DECLARE_BUFFER;
} render_buffer_t;

typedef struct render_vertexbuffer_t
{
	RENDER_DECLARE_BUFFER;
	render_vertex_decl_t          decl;
} render_vertexbuffer_t;

typedef struct render_indexbuffer_t
{
	RENDER_DECLARE_BUFFER;
} render_indexbuffer_t;

typedef struct render_shader_t
{
	RENDER_DECLARE_SHADER;
} render_shader_t;

typedef struct render_vertexshader_t
{
	RENDER_DECLARE_SHADER;
} render_vertexshader_t;

typedef struct render_pixelshader_t
{
	RENDER_DECLARE_SHADER;
} render_pixelshader_t;

// GLOBAL DATA

RENDER_EXTERN bool                _render_api_disabled[];
RENDER_EXTERN objectmap_t*        _render_map_target;
RENDER_EXTERN objectmap_t*        _render_map_buffer;
RENDER_EXTERN objectmap_t*        _render_map_shader;


// INTERNAL FUNCTIONS
RENDER_EXTERN int                 render_target_initialize( void );
RENDER_EXTERN void                render_target_shutdown( void );
RENDER_EXTERN object_t            render_target_create_framebuffer( render_backend_t* backend );
RENDER_EXTERN render_target_t*    render_target_lookup( object_t id );

