/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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

#include <foundation/foundation.h>

#include <render/render.h>
#include <render/internal.h>

#include <render/null/backend.h>
#include <render/metal/backend.h>

#include <resource/platform.h>

FOUNDATION_DECLARE_THREAD_LOCAL(render_backend_t*, backend, nullptr)

static render_api_t
render_api_fallback(render_api_t api) {
	switch (api) {
		case RENDERAPI_UNKNOWN:
			return RENDERAPI_UNKNOWN;

		case RENDERAPI_DEFAULT:
#if FOUNDATION_PLATFORM_WINDOWS
			return RENDERAPI_DIRECTX;
#elif FOUNDATION_PLATFORM_APPLE
			return RENDERAPI_METAL;
#else
			return RENDERAPI_NULL;
#endif

		case RENDERAPI_DIRECTX:
			return RENDERAPI_DIRECTX12;

		case RENDERAPI_DIRECTX12:
			return RENDERAPI_NULL;

		case RENDERAPI_METAL:
		case RENDERAPI_VULKAN:
		case RENDERAPI_COUNT:
			return RENDERAPI_NULL;

		case RENDERAPI_NULL:
			return RENDERAPI_UNKNOWN;

		default:
			break;
	}
	return RENDERAPI_UNKNOWN;
}

render_backend_t**
render_backends(void) {
	return render_backends_current;
}

render_backend_t*
render_backend_allocate(render_api_t api, bool allow_fallback) {
	// First find best matching supported backend
	render_backend_t* backend = 0;

	memory_context_push(HASH_RENDER);

	if (api == RENDERAPI_DEFAULT)
		allow_fallback = true;

	while (!backend) {
		while (render_api_disabled[api])
			api = render_api_fallback(api);
		switch (api) {
			case RENDERAPI_DIRECTX12:
				/*backend = render_dx11_allocate();
				if( !backend || !backend->vtable.construct( backend ) )
				{
				    log_info( HASH_RENDER, "Failed to initialize DirectX 11 render backend" );
				    render_deallocate( backend );
				    backend = nullptr;
				}*/
				break;

			case RENDERAPI_METAL:
#if FOUNDATION_PLATFORM_APPLE
				backend = render_backend_metal_allocate();
				if (!backend || !backend->vtable.construct(backend)) {
					log_info(HASH_RENDER, STRING_CONST("Failed to initialize Metal render backend"));
					render_backend_deallocate(backend);
					backend = nullptr;
				}
#endif
				break;

			case RENDERAPI_NULL:
				backend = render_backend_null_allocate();
				backend->vtable.construct(backend);
				break;

			case RENDERAPI_UNKNOWN:
				log_warn(HASH_RENDER, WARNING_SUSPICIOUS,
				         STRING_CONST("No supported and enabled render api found, giving up"));
				return 0;

			case RENDERAPI_VULKAN:
			case RENDERAPI_COUNT:
			case RENDERAPI_DEFAULT:
			case RENDERAPI_DIRECTX:
			default:
				// Try loading dynamic library
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

	backend->framecount = 1;

	uuidmap_initialize((uuidmap_t*)&backend->shadertable,
	                   sizeof(backend->shadertable.bucket) / sizeof(backend->shadertable.bucket[0]), 0);

	render_backend_set_resource_platform(backend, 0);

	array_push(render_backends_current, backend);

	memory_context_pop();

	set_thread_backend(backend);

	return backend;
}

void
render_backend_deallocate(render_backend_t* backend) {
	if (!backend)
		return;

	backend->vtable.destruct(backend);

	for (size_t ib = 0, bsize = array_size(render_backends_current); ib < bsize; ++ib) {
		if (render_backends_current[ib] == backend) {
			array_erase(render_backends_current, ib);
			break;
		}
	}

	memory_deallocate(backend);
}

render_api_t
render_backend_api(render_backend_t* backend) {
	return backend ? backend->api : RENDERAPI_UNKNOWN;
}

size_t
render_backend_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	return backend->vtable.enumerate_adapters(backend, store, capacity);
}

size_t
render_backend_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store,
                               size_t capacity) {
	return backend->vtable.enumerate_modes(backend, adapter, store, capacity);
}

uint64_t
render_backend_frame_count(render_backend_t* backend) {
	return backend->framecount;
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
	decl.render_api_group = (int)backend->api_group;
	decl.render_api = (int)backend->api;
	backend->platform = resource_platform(decl);
}

bool
render_backend_shader_upload(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	return backend->vtable.shader_upload(backend, shader, buffer, size);
}

void
render_backend_shader_finalize(render_backend_t* backend, render_shader_t* shader) {
	backend->vtable.shader_finalize(backend, shader);
}
