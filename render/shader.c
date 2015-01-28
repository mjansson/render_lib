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
#include <render/internal.h>


#define GET_SHADER( id ) objectmap_lookup( _render_map_shader, (id) )


object_t render_shader_ref( object_t id )
{
	int32_t ref;
	render_shader_t* shader = objectmap_lookup( _render_map_shader, id );
	if( shader ) do
	{
		ref = atomic_load32( &shader->ref );
		if( ( ref > 0 ) && atomic_cas32( &shader->ref, ref + 1, ref ) )
			return id;
	} while( ref > 0 );
	return 0;
}


void render_shader_destroy( object_t id )
{
	int32_t ref;
	render_shader_t* shader = GET_SHADER( id );
	if( shader ) do
	{
		ref = atomic_load32( &shader->ref );
		if( ( ref > 0 ) && atomic_cas32( &shader->ref, ref - 1, ref ) )
		{
			if( ref == 1 )
			{
				objectmap_free( _render_map_shader, id );
				shader->backend->vtable.deallocate_shader( shader->backend, (render_shader_t*)shader );
				memory_deallocate( shader );
			}
			return;
		}
	} while( ref > 0 );
}


object_t render_pixelshader_create( render_backend_t* backend )
{
	object_t id = objectmap_reserve( _render_map_shader );
	if( !id )
	{
		log_errorf( HASH_RENDER, ERROR_OUT_OF_MEMORY, "Unable to create pixel shader, out of slots in object map" );
		return 0;
	}
	
	memory_context_push( HASH_RENDER );
	
	render_pixelshader_t* shader = memory_allocate( HASH_RENDER, sizeof( render_pixelshader_t ), 16, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED );
	shader->id      = id;
	shader->backend = backend;
	shader->shadertype = SHADER_PIXEL;
	atomic_store32( &shader->ref, 1 );
	objectmap_set( _render_map_shader, id, shader );

	memory_context_pop();
	
	return shader->id;
}


void render_pixelshader_upload( object_t id, const void* buffer, unsigned int size )
{
	render_pixelshader_t* shader = GET_SHADER( id );
	shader->backend->vtable.upload_shader( shader->backend, (render_shader_t*)shader, buffer, size );
}


object_t render_vertexshader_create( render_backend_t* backend )
{
	object_t id = objectmap_reserve( _render_map_shader );
	if( !id )
	{
		log_errorf( HASH_RENDER, ERROR_OUT_OF_MEMORY, "Unable to create pixel shader, out of slots in object map" );
		return 0;
	}
	
	memory_context_push( HASH_RENDER );
	
	render_vertexshader_t* shader = memory_allocate( HASH_RENDER, sizeof( render_vertexshader_t ), 16, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED );
	shader->id      = id;
	shader->backend = backend;
	shader->shadertype = SHADER_VERTEX;
	atomic_store32( &shader->ref, 1 );
	objectmap_set( _render_map_shader, id, shader );
	
	memory_context_pop();
	
	return shader->id;
}


void render_vertexshader_upload( object_t id, const void* buffer, unsigned int size )
{
	render_vertexshader_t* shader = GET_SHADER( id );
	shader->backend->vtable.upload_shader( shader->backend, (render_shader_t*)shader, buffer, size );
}


uuid_t render_shader_guid( object_t id )
{
	render_shader_t* shader = GET_SHADER( id );
	return shader ? shader->uuid : uuid_null();
}


void render_shader_set_uuid( object_t id, const uuid_t uuid )
{
	render_shader_t* shader = GET_SHADER( id );
	if( shader )
		shader->uuid = uuid;
}

