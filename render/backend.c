/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

#include <render/null/backend.h>
#include <render/gl2/backend.h>
#include <render/gl4/backend.h>
#include <render/gles2/backend.h>

#include <resource/platform.h>

RENDER_EXTERN render_config_t _render_config;

FOUNDATION_DECLARE_THREAD_LOCAL(render_backend_t*, backend, nullptr)

static render_api_t
render_api_fallback(render_api_t api) {
	switch (api) {
	case RENDERAPI_UNKNOWN:    return RENDERAPI_UNKNOWN;

	case RENDERAPI_DEFAULT:
#if FOUNDATION_PLATFORM_WINDOWS
		return RENDERAPI_DIRECTX;
#elif FOUNDATION_PLATFORM_IOS || FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
		return RENDERAPI_GLES;
#else
		return RENDERAPI_OPENGL;
#endif

	case RENDERAPI_NULL:      return RENDERAPI_UNKNOWN;

	case RENDERAPI_OPENGL:    return RENDERAPI_OPENGL4;
	case RENDERAPI_DIRECTX:   return RENDERAPI_DIRECTX11;
	case RENDERAPI_GLES:      return RENDERAPI_GLES3;

	case RENDERAPI_OPENGL3:   return RENDERAPI_OPENGL2;
#if FOUNDATION_PLATFORM_WINDOWS
	case RENDERAPI_OPENGL4:   return RENDERAPI_DIRECTX10;
#else
	case RENDERAPI_OPENGL4:   return RENDERAPI_OPENGL3;
#endif
	case RENDERAPI_DIRECTX10: return RENDERAPI_OPENGL3;
	case RENDERAPI_DIRECTX11: return RENDERAPI_OPENGL4;
	case RENDERAPI_GLES3:     return RENDERAPI_GLES2;
	case RENDERAPI_GLES2:     return RENDERAPI_NULL;
	case RENDERAPI_OPENGL2:   return RENDERAPI_NULL;

	case RENDERAPI_PS3:
	case RENDERAPI_PS4:
	case RENDERAPI_XBOX360:
	case RENDERAPI_XBOXONE:
	case RENDERAPI_NUM:       return RENDERAPI_NULL;

	default:                  break;
	}
	return RENDERAPI_UNKNOWN;
}

render_backend_t*
render_backend_allocate(render_api_t api, bool allow_fallback) {
	//First find best matching supported backend
	render_backend_t* backend = 0;

	memory_context_push(HASH_RENDER);

	while (!backend) {
		while (_render_api_disabled[api]) api = render_api_fallback(api);
		switch (api) {
		case RENDERAPI_GLES2:
			backend = render_backend_gles2_allocate();
			if (!backend || !backend->vtable.construct(backend)) {
				log_info(HASH_RENDER, STRING_CONST("Failed to initialize OpenGL ES 2 render backend"));
				render_backend_deallocate(backend);
				backend = 0;
			}
			break;

		case RENDERAPI_GLES3:
			/*backend = render_backend_gles3_allocate();
			if (!backend || !backend->vtable.construct(backend)) {
				log_info(HASH_RENDER, STRING_CONST("Failed to initialize OpenGL ES 3 render backend"));
				render_backend_deallocate(backend), backend = 0;
			}*/
			break;

		case RENDERAPI_OPENGL2:
			backend = render_backend_gl2_allocate();
			if (!backend || !backend->vtable.construct(backend)) {
				log_info(HASH_RENDER, STRING_CONST("Failed to initialize OpenGL 2 render backend"));
				render_backend_deallocate(backend);
				backend = 0;
			}
			break;

		case RENDERAPI_OPENGL3:
			/*backend = render_gl3_allocate();
			if( !backend || !backend->vtable.construct( backend ) )
			{
				log_info( HASH_RENDER, "Failed to initialize OpenGL 3 render backend" );
				render_deallocate( backend ), backend = 0;
			}*/
			break;

		case RENDERAPI_OPENGL4:
			/*backend = render_backend_gl4_allocate();
			if (!backend || !backend->vtable.construct(backend)) {
				log_info(HASH_RENDER, STRING_CONST("Failed to initialize OpenGL 4 render backend"));
				render_backend_deallocate(backend), backend = 0;
			}*/
			break;

		case RENDERAPI_DIRECTX10:
			/*backend = render_dx10_allocate();
			if( !backend || !backend->vtable.construct( backend ) )
			{
				log_info( HASH_RENDER, "Failed to initialize DirectX 10 render backend" );
				render_deallocate( backend ), backend = 0;
			}*/
			break;

		case RENDERAPI_DIRECTX11:
			/*backend = render_dx11_allocate();
			if( !backend || !backend->vtable.construct( backend ) )
			{
				log_info( HASH_RENDER, "Failed to initialize DirectX 11 render backend" );
				render_deallocate( backend ), backend = 0;
			}*/
			break;

		case RENDERAPI_NULL:
			backend = render_backend_null_allocate();
			backend->vtable.construct(backend);
			break;

		case RENDERAPI_UNKNOWN:
			log_warn(HASH_RENDER, WARNING_SUSPICIOUS,
			         STRING_CONST("No supported and enabled render api found, giving up"));
			return 0;

		case RENDERAPI_PS3:
		case RENDERAPI_PS4:
		case RENDERAPI_XBOX360:
		case RENDERAPI_XBOXONE:
			//Try loading dynamic library
			log_warnf(HASH_RENDER, WARNING_SUSPICIOUS,
			          STRING_CONST("Render API not yet implemented (%u)"), api);
			break;
		
		case RENDERAPI_NUM:
		case RENDERAPI_DEFAULT:
		case RENDERAPI_OPENGL:
		case RENDERAPI_DIRECTX:
		case RENDERAPI_GLES:
		default:
			//Try loading dynamic library
			log_warnf(HASH_RENDER, WARNING_SUSPICIOUS,
			          STRING_CONST("Unknown render API (%u), dynamic library loading not implemented yet"), api);
			break;
		}

		if (!backend) {
			if (!allow_fallback) {
				log_warn(HASH_RENDER, WARNING_UNSUPPORTED, STRING_CONST("Requested render api not supported"));
				return 0;
			}

			api = render_api_fallback(api);
		}
	}

	backend->framebuffer = render_target_create_framebuffer(backend);
	backend->framecount = 1;

	backend->shadertable = hashtable64_allocate(_render_config.shader_max);
	backend->programtable = hashtable64_allocate(_render_config.program_max);

	backend->shadermap = objectmap_allocate(_render_config.shader_max);
	backend->programmap = objectmap_allocate(_render_config.program_max);

	render_backend_set_resource_platform(backend, 0);

	memory_context_pop();

	return backend;
}

void
render_backend_deallocate(render_backend_t* backend) {
	if (!backend)
		return;

	backend->vtable.destruct(backend);

	hashtable64_deallocate(backend->shadertable);
	hashtable64_deallocate(backend->programtable);

	objectmap_deallocate(backend->shadermap);
	objectmap_deallocate(backend->programmap);

	if (backend->framebuffer)
		render_target_destroy(backend->framebuffer);

	memory_deallocate(backend);
}

render_api_t
render_backend_api(render_backend_t* backend) {
	return backend ? backend->api : RENDERAPI_UNKNOWN;
}

unsigned int*
render_backend_enumerate_adapters(render_backend_t* backend) {
	return backend->vtable.enumerate_adapters(backend);
}

render_resolution_t*
render_backend_enumerate_modes(render_backend_t* backend, unsigned int adapter) {
	return backend->vtable.enumerate_modes(backend, adapter);
}

void
render_backend_set_format(render_backend_t* backend, const pixelformat_t format,
                          const colorspace_t space) {
	if (!FOUNDATION_VALIDATE_MSG(!backend->drawable,
	                             "Unable to change format when drawable is already set"))
		return;
	backend->pixelformat = format;
	backend->colorspace  = space;
}

void
render_backend_set_drawable(render_backend_t* backend, render_drawable_t* drawable) {
	render_target_t* framebuffer_target;
	render_drawable_t* old = backend->drawable;
	if (old == drawable)
		return;

	if (!backend->vtable.set_drawable(backend, drawable))
		return;

	backend->drawable = drawable;
	if (old)
		render_drawable_deallocate(old);

	framebuffer_target = render_target_lookup(backend->framebuffer);
	if (framebuffer_target) {
		framebuffer_target->width = render_drawable_width(drawable);
		framebuffer_target->height = render_drawable_height(drawable);
		framebuffer_target->pixelformat = backend->pixelformat;
		framebuffer_target->colorspace = backend->colorspace;
	}

	set_thread_backend(backend);
}

render_drawable_t*
render_backend_drawable(render_backend_t* backend) {
	return backend->drawable;
}

object_t
render_backend_target_framebuffer(render_backend_t* backend) {
	return backend->framebuffer;
}

void
render_backend_dispatch(render_backend_t* backend, render_context_t** contexts,
                        size_t num_contexts) {
	backend->vtable.dispatch(backend, contexts, num_contexts);

	for (size_t i = 0; i < num_contexts; ++i)
		atomic_store32(&contexts[i]->reserved, 0);
}

void
render_backend_flip(render_backend_t* backend) {
	backend->vtable.flip(backend);
}

uint64_t
render_backend_frame_count(render_backend_t* backend) {
	return backend->framecount;
}

void
render_backend_enable_thread(render_backend_t* backend) {
	backend->vtable.enable_thread(backend);
	set_thread_backend(backend);
}

void
render_backend_disable_thread(render_backend_t* backend) {
	backend->vtable.disable_thread(backend);
	set_thread_backend(nullptr);
}

render_backend_t*
render_backend_thread(void) {
	return get_thread_backend();
}

uint64_t
render_backend_resource_platform(render_backend_t* backend) {
	return backend->platform;
}

void
render_backend_set_resource_platform(render_backend_t* backend, uint64_t platform) {
	resource_platform_t decl = resource_platform_decompose(platform);
	decl.render_api_group = backend->api_group;
	decl.render_api = backend->api;
	backend->platform = resource_platform(decl);
}

object_t
render_backend_shader_acquire(render_backend_t* backend, const uuid_t uuid) {
	object_t obj = hashtable64_get(backend->shadertable, resource_uuid_hash(uuid));
	return objectmap_lookup_ref(backend->shadermap, obj) ? obj : 0;
}

object_t
render_backend_program_acquire(render_backend_t* backend, const uuid_t uuid) {
	object_t obj = hashtable64_get(backend->programtable, resource_uuid_hash(uuid));
	return objectmap_lookup_ref(backend->programmap, obj) ? obj : 0;
}

object_t
render_backend_shader_store(render_backend_t* backend, const uuid_t uuid,
                            render_shader_t* shader) {
	object_t obj = objectmap_reserve(backend->shadermap);
	if (obj) {
		atomic_store32(&shader->ref, 1);
		shader->id = obj;
		shader->flags = 0;
		objectmap_set(backend->shadermap, obj, shader);
		hashtable64_set(backend->shadertable, resource_uuid_hash(uuid), obj);
	}
	return obj;
}

object_t
render_backend_program_store(render_backend_t* backend, const uuid_t uuid,
                             render_program_t* program) {
	object_t obj = objectmap_reserve(backend->programmap);
	if (obj) {
		atomic_store32(&program->ref, 1);
		program->id = obj;
		program->flags = 0;
		objectmap_set(backend->programmap, obj, program);
		hashtable64_set(backend->programtable, resource_uuid_hash(uuid), obj);
	}
	return obj;
}

render_shader_t*
render_backend_shader_resolve(render_backend_t* backend, object_t shader) {
	return objectmap_lookup(backend->shadermap, shader);
}

render_program_t*
render_backend_program_resolve(render_backend_t* backend, object_t program) {
	return objectmap_lookup(backend->programmap, program);
}

static void
render_backend_shader_deallocate(object_t id, void* shader) {
	render_shader_deallocate(shader);
	objectmap_free(((render_shader_t*)shader)->backend->shadermap, id);
}

void
render_backend_shader_release(render_backend_t* backend, object_t shader) {
	objectmap_lookup_unref(backend->shadermap, shader, render_backend_shader_deallocate);
}

static void
render_backend_program_deallocate(object_t id, void* program) {
	render_program_deallocate(program);
	objectmap_free(((render_program_t*)program)->backend->programmap, id);
}

void
render_backend_program_release(render_backend_t* backend, object_t program) {
	objectmap_lookup_unref(backend->programmap, program, render_backend_program_deallocate);
}
