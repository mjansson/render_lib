/* types.h  -  Render library  -  Public Domain  -  2013 Mattias Jansson
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

#pragma once

/*! \file types.h
    Render data types */

#include <foundation/platform.h>
#include <foundation/types.h>

#include <window/types.h>
#include <resource/types.h>
#include <task/types.h>
#include <vector/types.h>

#include <render/build.h>

typedef enum render_api_t {
	RENDERAPI_UNKNOWN = 0,
	RENDERAPI_DEFAULT,
	RENDERAPI_NULL,
	RENDERAPI_DIRECTX,
	RENDERAPI_DIRECTX12,
	RENDERAPI_METAL,
	RENDERAPI_VULKAN,

	RENDERAPI_COUNT
} render_api_t;

typedef enum render_api_group_t {
	RENDERAPIGROUP_NONE = 0,
	RENDERAPIGROUP_DIRECTX,
	RENDERAPIGROUP_METAL,
	RENDERAPIGROUP_VULKAN,

	RENDERAPIGROUP_COUNT
} render_api_group_t;

typedef enum render_target_type_t {
	RENDERTARGET_INVALID = 0,
	RENDERTARGET_WINDOW,
	RENDERTARGET_FULLSCREEN,
	RENDERTARGET_TEXTURE,

	RENDERTARGET_TYPECOUNT,
	RENDERTARGET_UNKNOWN = 0x7FFFFFFF
} render_target_type_t;

typedef enum render_pixelformat_t {
	PIXELFORMAT_INVALID = 0,

	PIXELFORMAT_R8G8B8,
	PIXELFORMAT_R8G8B8A8,

	PIXELFORMAT_R16G16B16,
	PIXELFORMAT_R16G16B16A16,

	PIXELFORMAT_R32G32B32F,
	PIXELFORMAT_R32G32B32A32F,

	PIXELFORMAT_A8,

	PIXELFORMAT_DEPTH32F,

	PIXELFORMAT_COUNT,
	PIXELFORMAT_UNKNOWN = 0x7FFFFFFF
} render_pixelformat_t;

typedef enum render_colorspace_t {
	COLORSPACE_INVALID = 0,

	COLORSPACE_LINEAR,
	COLORSPACE_sRGB,

	COLORSPACE_COUNT,
	COLORSPACE_UNKNOWN = 0x7fffffff
} render_colorspace_t;

typedef enum render_indexformat_t { RENDER_INDEXFORMAT_UINT16 = 0, RENDER_INDEXFORMAT_UINT32 = 1 } render_indexformat_t;

typedef enum render_clear_action_t {
	//! Let driver decide, content is undefined
	RENDERCLEAR_DONTCARE = 0,
	//! Preserve previous content
	RENDERCLEAR_PRESERVE,
	//! Clear to set color value
	RENDERCLEAR_CLEAR
} render_clear_action_t;

typedef enum render_usage_t {
	RENDERUSAGE_DEFAULT = 0,
	RENDERUSAGE_CPUONLY = 0x01,
	RENDERUSAGE_GPUONLY = 0x02,
	RENDERUSAGE_TARGET = 0x04,
	RENDERUSAGE_RENDER = 0x08
} render_usage_t;

typedef enum render_buffer_flag_t {
	RENDERBUFFER_DIRTY = 0x01,
	RENDERBUFFER_LOST = 0x02,

	RENDERBUFFER_LOCK_READ = 0x10,
	RENDERBUFFER_LOCK_WRITE = 0x20,
	RENDERBUFFER_LOCK_DISCARD = 0x40,
	RENDERBUFFER_LOCK_WRITE_ALL = 0x60,
	RENDERBUFFER_LOCK_BITS = 0xF0
} render_buffer_flag_t;

typedef enum render_primitive_type { RENDERPRIMITIVE_TRIANGLELIST = 0 } render_primitive_type;

typedef enum render_data_type { RENDERDATA_POINTER, RENDERDATA_FLOAT4, RENDERDATA_MATRIX4X4 } render_data_type;

#define RENDER_TARGET_COLOR_ATTACHMENT_COUNT 4

typedef struct render_config_t render_config_t;
typedef struct render_backend_vtable_t render_backend_vtable_t;
typedef struct render_backend_t render_backend_t;
typedef struct render_resolution_t render_resolution_t;
typedef struct render_target_t render_target_t;
typedef struct render_pipeline_t render_pipeline_t;
typedef struct render_shader_t render_shader_t;
typedef struct render_buffer_t render_buffer_t;
typedef struct render_primitive_t render_primitive_t;
typedef struct render_buffer_data_t render_buffer_data_t;
typedef struct render_argument_t render_argument_t;

typedef uint32_t render_pipeline_state_t;
typedef uint32_t render_buffer_index_t;
typedef uint32_t render_count_t;
typedef uint32_t render_offset_t;

typedef bool (*render_backend_construct_fn)(render_backend_t*);
typedef void (*render_backend_destruct_fn)(render_backend_t*);
typedef size_t (*render_backend_enumerate_adapters_fn)(render_backend_t*, uint*, size_t);
typedef size_t (*render_backend_enumerate_modes_fn)(render_backend_t*, uint, render_resolution_t*, size_t);
typedef render_target_t* (*render_backend_target_window_allocate_fn)(render_backend_t*, window_t*, uint);
typedef render_target_t* (*render_backend_target_texture_allocate_fn)(render_backend_t*, uint, uint,
                                                                      render_pixelformat_t);
typedef void (*render_backend_target_deallocate_fn)(render_backend_t*, render_target_t*);
typedef render_pipeline_t* (*render_backend_pipeline_allocate_fn)(render_backend_t*, render_indexformat_t, uint);
typedef void (*render_backend_pipeline_deallocate_fn)(render_backend_t*, render_pipeline_t*);
typedef void (*render_backend_pipeline_set_color_attachment_fn)(render_backend_t*, render_pipeline_t*, uint,
                                                                render_target_t*);
typedef void (*render_backend_pipeline_set_depth_attachment_fn)(render_backend_t*, render_pipeline_t*,
                                                                render_target_t*);
typedef void (*render_backend_pipeline_set_color_clear_fn)(render_backend_t*, render_pipeline_t*, uint,
                                                           render_clear_action_t, vector_t);
typedef void (*render_backend_pipeline_set_depth_clear_fn)(render_backend_t*, render_pipeline_t*, render_clear_action_t,
                                                           vector_t);
typedef void (*render_backend_pipeline_build_fn)(render_backend_t*, render_pipeline_t*);
typedef render_pipeline_state_t (*render_backend_pipeline_state_allocate_fn)(render_backend_t*, render_pipeline_t*,
                                                                             render_shader_t*);
typedef void (*render_backend_pipeline_state_deallocate_fn)(render_backend_t*, render_pipeline_state_t);
typedef void (*render_backend_pipeline_flush_fn)(render_backend_t*, render_pipeline_t*);
typedef void (*render_backend_pipeline_use_buffer_fn)(render_backend_t*, render_pipeline_t*, render_buffer_index_t);
typedef bool (*render_backend_shader_upload_fn)(render_backend_t*, render_shader_t*, const void*, size_t);
typedef void (*render_backend_shader_finalize_fn)(render_backend_t*, render_shader_t*);
typedef void (*render_backend_buffer_allocate_fn)(render_backend_t*, render_buffer_t*, size_t, const void*, size_t);
typedef void (*render_backend_buffer_deallocate_fn)(render_backend_t*, render_buffer_t*, bool, bool);
typedef void (*render_backend_buffer_upload_fn)(render_backend_t*, render_buffer_t*, size_t, size_t);
typedef void (*render_backend_buffer_set_label_fn)(render_backend_t*, render_buffer_t*, const char*, size_t);
typedef void (*render_backend_buffer_data_declare_fn)(render_backend_t*, render_buffer_t*, size_t,
                                                      const render_buffer_data_t*, size_t);
typedef void (*render_backend_buffer_data_encode_buffer_fn)(render_backend_t*, render_buffer_t*, uint, uint,
                                                            render_buffer_t*, uint);
typedef void (*render_backend_buffer_data_encode_matrix_fn)(render_backend_t*, render_buffer_t*, uint, uint,
                                                            const matrix_t*);
typedef void (*render_backend_buffer_data_encode_constant_fn)(render_backend_t*, render_buffer_t*, uint, uint,
                                                              const void*, uint);

struct render_config_t {
	uint unused;
};

struct render_backend_vtable_t {
	render_backend_construct_fn construct;
	render_backend_destruct_fn destruct;
	render_backend_enumerate_adapters_fn enumerate_adapters;
	render_backend_enumerate_modes_fn enumerate_modes;
	render_backend_target_window_allocate_fn target_window_allocate;
	render_backend_target_texture_allocate_fn target_texture_allocate;
	render_backend_target_deallocate_fn target_deallocate;
	render_backend_pipeline_allocate_fn pipeline_allocate;
	render_backend_pipeline_deallocate_fn pipeline_deallocate;
	render_backend_pipeline_set_color_attachment_fn pipeline_set_color_attachment;
	render_backend_pipeline_set_depth_attachment_fn pipeline_set_depth_attachment;
	render_backend_pipeline_set_color_clear_fn pipeline_set_color_clear;
	render_backend_pipeline_set_depth_clear_fn pipeline_set_depth_clear;
	render_backend_pipeline_build_fn pipeline_build;
	render_backend_pipeline_flush_fn pipeline_flush;
	render_backend_pipeline_use_buffer_fn pipeline_use_argument_buffer;
	render_backend_pipeline_use_buffer_fn pipeline_use_render_buffer;
	render_backend_pipeline_state_allocate_fn pipeline_state_allocate;
	render_backend_pipeline_state_deallocate_fn pipeline_state_deallocate;
	render_backend_shader_upload_fn shader_upload;
	render_backend_shader_finalize_fn shader_finalize;
	render_backend_buffer_allocate_fn buffer_allocate;
	render_backend_buffer_deallocate_fn buffer_deallocate;
	render_backend_buffer_upload_fn buffer_upload;
	render_backend_buffer_set_label_fn buffer_set_label;
	render_backend_buffer_data_declare_fn buffer_data_declare;
	render_backend_buffer_data_encode_buffer_fn buffer_data_encode_buffer;
	render_backend_buffer_data_encode_matrix_fn buffer_data_encode_matrix;
	render_backend_buffer_data_encode_constant_fn buffer_data_encode_constant;
};

#if FOUNDATION_SIZE_POINTER == 4
#define RENDER_32BIT_PADDING(name) uint32_t __pad_##name;
#define RENDER_32BIT_PADDING_ARR(name, cnt) uint32_t __pad_ #name[cnt];
#else
#define RENDER_32BIT_PADDING(...)
#define RENDER_32BIT_PADDING_ARR(...)
#endif

struct render_backend_t {
	render_api_t api;
	render_api_group_t api_group;
	render_backend_vtable_t vtable;
	uint64_t framecount;
	uint64_t platform;
	uuidmap_fixed_t shader_table;
	hash_t shader_type;
};

struct render_resolution_t {
	uint id;
	uint width;
	uint height;
	render_pixelformat_t pixelformat;
	uint refresh;
};

struct render_target_t {
	render_backend_t* backend;
	render_target_type_t type;
	uint width;
	uint height;
	render_pixelformat_t pixelformat;
	render_colorspace_t colorspace;
};

struct render_pipeline_t {
	render_backend_t* backend;
	render_target_t* color_attachment[RENDER_TARGET_COLOR_ATTACHMENT_COUNT];
	render_target_t* depth_attachment;
	render_buffer_t* primitive_buffer;
	render_indexformat_t index_format;
	atomic32_t primitive_used;
	atomic32_t* barrier;
};

struct render_shader_t {
	render_backend_t* backend;
	RENDER_32BIT_PADDING(backendptr)
	atomic32_t ref;
	uint32_t unused;
	uuid_t uuid;
	uintptr_t backend_data[4];
	RENDER_32BIT_PADDING_ARR(backend_data, 4)
};

struct render_buffer_t {
	render_backend_t* backend;
	RENDER_32BIT_PADDING(backendptr)
	render_buffer_index_t render_index;
	uint8_t usage;
	uint8_t buffertype;
	uint8_t flags;
	uint8_t unused_byte;
	uint32_t locks;
	size_t allocated;
	size_t used;
	void* store;
	void* access;
	uintptr_t backend_data[4];
	semaphore_t lock;
};

//! Buffer structured data
struct render_buffer_data_t {
	uint index;
	render_data_type data_type;
	uint array_count;
};

struct render_argument_t {
	render_count_t index_count;
	render_count_t instance_count;
	render_offset_t index_offset;
	render_offset_t vertex_base;
	render_offset_t instance_base;
	// render_offset_t padding[3];
};

struct render_primitive_t {
	render_pipeline_state_t pipeline_state;
	render_buffer_index_t argument_buffer;
	render_offset_t argument_offset;
	render_buffer_index_t index_buffer;
	render_buffer_index_t descriptor[4];
};
