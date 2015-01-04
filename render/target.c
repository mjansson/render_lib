/* target.c  -  Render library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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

#include <render/render.h>
#include <render/internal.h>


objectmap_t* _render_map_target = 0;


int render_target_initialize( void )
{
	memory_context_push( HASH_RENDER );
	_render_map_target = objectmap_allocate( BUILD_SIZE_RENDER_TARGET_MAP );
	memory_context_pop();
	return 0;
}


void render_target_shutdown( void )
{
	objectmap_deallocate( _render_map_target );
	_render_map_target = 0;
}


object_t render_target_create( render_backend_t* backend )
{
	render_target_t* target;
	object_t id;
	
	memory_context_push( HASH_RENDER );

	id = objectmap_reserve( _render_map_target );
	if( !id )
	{
		log_errorf( HASH_RENDER, ERROR_OUT_OF_MEMORY, "Unable to create render target, out of slots in object map" );
		return 0;
	}
	
	target = memory_allocate( HASH_RENDER, sizeof( render_target_t ), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED );
	target->id = id;
	target->backend = backend;
	atomic_store32( &target->ref, 1 );
	
	objectmap_set( _render_map_target, id, target );
	
	return id;
}


object_t render_target_create_framebuffer( render_backend_t* backend )
{
	return render_target_create( backend );
}


object_t render_target_ref( object_t id )
{
	int32_t ref;
	render_target_t* target = objectmap_lookup( _render_map_target, id );
	if( target ) do
	{
		ref = atomic_load32( &target->ref );
		if( ( ref > 0 ) && atomic_cas32( &target->ref, ref + 1, ref ) )
			return id;
	} while( ref > 0 );
	return 0;
}


void render_target_destroy( object_t id )
{
	int32_t ref;
	render_target_t* target = objectmap_lookup( _render_map_target, id );
	if( target ) do
	{
		ref = atomic_load32( &target->ref );
		if( ( ref > 0 ) && atomic_cas32( &target->ref, ref - 1, ref ) )
		{
			if( ref == 1 )
			{
				objectmap_free( _render_map_target, id );
				memory_deallocate( target );
			}
			return;
		}
	} while( ref > 0 );
}


render_target_t* render_target_lookup( object_t id )
{
	return objectmap_lookup( _render_map_target, id );
}


unsigned int render_target_width( object_t id )
{
	render_target_t* target = objectmap_lookup( _render_map_target, id );
	if( !target )
		return 0;
	return target->width;
}


unsigned int render_target_height( object_t id )
{
	render_target_t* target = objectmap_lookup( _render_map_target, id );
	if( !target )
		return 0;
	return target->height;
}


pixelformat_t render_target_pixelformat( object_t id )
{
	render_target_t* target = objectmap_lookup( _render_map_target, id );
	if( !target )
		return PIXELFORMAT_INVALID;
	return target->pixelformat;
}


colorspace_t render_target_colorspace( object_t id )
{
	render_target_t* target = objectmap_lookup( _render_map_target, id );
	if( !target )
		return COLORSPACE_INVALID;
	return target->colorspace;
}

