/* parameter.h  -  Render library  -  Public Domain  -  2015 Mattias Jansson / Rampant Pixels
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

/*! \file parameter.h
    Programmable render pipeline parameters */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_parameter_decl_t*
render_parameter_decl_allocate(void);

RENDER_API void
render_parameter_decl_initialize(render_parameter_decl_t* decl);

RENDER_API void
render_parameter_decl_finalize(render_parameter_decl_t* decl);

RENDER_API void
render_parameter_decl_deallocate(render_parameter_decl_t* decl);


RENDER_API object_t
render_parameterbuffer_create(render_backend_t* backend, const render_parameter_decl_t* decl, const void* data);

RENDER_API object_t
render_parameterbuffer_ref(object_t buffer);

RENDER_API void
render_parameterbuffer_destroy(object_t buffer);

RENDER_API const render_parameter_decl_t*
render_parameterbuffer_decl(object_t buffer);

RENDER_API void
render_parameterbuffer_lock(object_t buffer, unsigned int lock);

RENDER_API void
render_parameterbuffer_unlock(object_t buffer);

RENDER_API render_buffer_uploadpolicy_t
render_parameterbuffer_upload_policy(object_t buffer);

RENDER_API void
render_parameterbuffer_set_upload_policy(object_t buffer, render_buffer_uploadpolicy_t policy);

RENDER_API void
render_parameterbuffer_upload(object_t buffer);

RENDER_API void
render_parameterbuffer_release(object_t buffer, bool sys, bool aux);

RENDER_API void
render_parameterbuffer_restore(object_t buffer);
