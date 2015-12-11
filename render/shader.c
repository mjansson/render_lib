/* shader.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

render_pixelshader_t*
render_pixelshader_allocate(void) {
	render_pixelshader_t* shader = memory_allocate(HASH_RENDER, sizeof(render_pixelshader_t), 0,
	                                               MEMORY_PERSISTENT);
	render_pixelshader_initialize(shader);
	return shader;
}

void
render_pixelshader_initialize(render_pixelshader_t* shader) {
	memset(shader, 0, sizeof(render_pixelshader_t));
	shader->shadertype = SHADER_PIXEL;
}

void
render_pixelshader_finalize(render_pixelshader_t* shader) {
	if (shader->backend)
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
}

void
render_pixelshader_deallocate(render_pixelshader_t* shader) {
	if (shader)
		render_pixelshader_finalize(shader);
	memory_deallocate(shader);
}

void
render_pixelshader_upload(render_pixelshader_t* shader, const void* buffer, size_t size) {
	shader->backend->vtable.upload_shader(shader->backend, (render_shader_t*)shader, buffer, size);
}

render_vertexshader_t*
render_vertexshader_allocate(void) {
	render_vertexshader_t* shader = memory_allocate(HASH_RENDER, sizeof(render_vertexshader_t), 16,
	                                                MEMORY_PERSISTENT);
	render_vertexshader_initialize(shader);
	return shader;
}

void
render_vertexshader_initialize(render_vertexshader_t* shader) {
	memset(shader, 0, sizeof(render_vertexshader_t));
	shader->shadertype = SHADER_VERTEX;
}

void
render_vertexshader_finalize(render_vertexshader_t* shader) {
	if (shader->backend)
		shader->backend->vtable.deallocate_shader(shader->backend, (render_shader_t*)shader);
}

void
render_vertexshader_deallocate(render_vertexshader_t* shader) {
	if (shader)
		render_vertexshader_finalize(shader);
	memory_deallocate(shader);
}

void
render_vertexshader_upload(render_vertexshader_t* shader, const void* buffer, size_t size) {
	shader->backend->vtable.upload_shader(shader->backend, (render_shader_t*)shader, buffer, size);
}
