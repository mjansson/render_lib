/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The foundation library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>
#include <window/window.h>
#include <render/render.h>
#include <render/internal.h>


static bool                    _rb_null_construct( render_backend_t* backend ) { log_debug( HASH_RENDER, "Constructed NULL render backend" ); return true; }
static void                    _rb_null_destruct( render_backend_t* backend ) { log_debug( HASH_RENDER, "Destructed NULL render backend" ); }
static unsigned int*           _rb_null_enumerate_adapters( render_backend_t* backend ) { unsigned int* adapters = 0; array_push( adapters, WINDOW_ADAPTER_DEFAULT ); return adapters; }
static render_resolution_t*    _rb_null_enumerate_modes( render_backend_t* backend, unsigned int adapter ) { render_resolution_t* modes = 0; render_resolution_t mode = { 0, 800, 600, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR, 60 }; array_push( modes, mode ); return modes; }
static bool                    _rb_null_set_drawable( render_backend_t* backend, render_drawable_t* drawable ) { return true; }
static void                    _rb_null_dispatch( render_backend_t* backend, render_context_t** contexts, unsigned int num_contexts ) {}
static void                    _rb_null_flip( render_backend_t* backend ) { ++backend->framecount; }
static void*                   _rb_null_allocate_buffer( render_backend_t* backend, render_buffer_t* buffer ) { return memory_allocate( HASH_RENDER, buffer->size * buffer->allocated, 16, MEMORY_PERSISTENT ); }
static void                    _rb_null_deallocate_buffer( render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux ) { if( sys ) memory_deallocate( buffer->store ); }
static void                    _rb_null_upload_buffer( render_backend_t* backend, render_buffer_t* buffer ) {}
static void                    _rb_null_upload_shader( render_backend_t* backend, render_shader_t* shader, const void* buffer, unsigned int size ) {}
static void*                   _rb_null_read_shader( render_backend_t* backend, render_shader_t* shader, uint64_t* size ) { return 0; }
static void                    _rb_null_deallocate_shader( render_backend_t* backend, render_shader_t* shader ) {}


static render_backend_vtable_t _render_backend_vtable_null = {
	.construct = _rb_null_construct,
	.destruct  = _rb_null_destruct,
	.enumerate_adapters = _rb_null_enumerate_adapters,
	.enumerate_modes = _rb_null_enumerate_modes,
	.set_drawable = _rb_null_set_drawable,
	.dispatch = _rb_null_dispatch,
	.flip = _rb_null_flip,
	.allocate_buffer = _rb_null_allocate_buffer,
	.deallocate_buffer = _rb_null_deallocate_buffer,
	.upload_buffer = _rb_null_upload_buffer,
	.upload_shader = _rb_null_upload_shader,
	.read_shader = _rb_null_read_shader,
	.deallocate_shader = _rb_null_deallocate_shader
};


render_backend_t* render_backend_null_allocate()
{
	render_backend_t* backend = memory_allocate( HASH_RENDER, sizeof( render_backend_t ), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED );
	backend->api = RENDERAPI_NULL;
	backend->vtable = _render_backend_vtable_null;
	return backend;
}

