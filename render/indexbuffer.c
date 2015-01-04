/* indexbuffer.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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


#define GET_BUFFER( id ) objectmap_lookup( _render_map_buffer, (id) )


object_t render_indexbuffer_create( render_backend_t* backend, render_usage_t usage, unsigned int indices, const uint16_t* data )
{
	object_t id = objectmap_reserve( _render_map_buffer );
	if( !id )
	{
		log_errorf( HASH_RENDER, ERROR_OUT_OF_MEMORY, "Unable to create index buffer, out of slots in object map" );
		return 0;
	}

	memory_context_push( HASH_RENDER );
	
	render_indexbuffer_t* buffer = memory_allocate( HASH_RENDER, sizeof( render_indexbuffer_t ), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED );
	buffer->id         = id;
	buffer->backend    = backend;
	buffer->usage      = usage;
	buffer->buffertype = RENDERBUFFER_INDEX;
	buffer->policy     = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->size       = 2;
	atomic_store32( &buffer->ref, 1 );
	objectmap_set( _render_map_buffer, id, buffer );
	
	if( indices )
	{
		buffer->allocated = indices;
		buffer->used = indices;
		buffer->store = backend->vtable.allocate_buffer( backend, (render_buffer_t*)buffer );
		if( data )
		{
			memcpy( buffer->store, data, indices * buffer->size );
			buffer->flags |= RENDERBUFFER_DIRTY;
		}
	}
	
	memory_context_pop();
	
	return buffer->id;
}


object_t render_indexbuffer_load( const uuid_t uuid )
{
	return 0;
}


object_t render_indexbuffer_ref( object_t id )
{
	int32_t ref;
	render_indexbuffer_t* buffer = objectmap_lookup( _render_map_buffer, id );
	if( buffer ) do
	{
		ref = atomic_load32( &buffer->ref );
		if( ( ref > 0 ) && atomic_cas32( &buffer->ref, ref + 1, ref ) )
			return id;
	} while( ref > 0 );
	return 0;
}


void render_indexbuffer_destroy( object_t id )
{
	int32_t ref;
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer ) do
	{
		ref = atomic_load32( &buffer->ref );
		if( ( ref > 0 ) && atomic_cas32( &buffer->ref, ref - 1, ref ) )
		{
			if( ref == 1 )
			{
				objectmap_free( _render_map_buffer, id );
				buffer->backend->vtable.deallocate_buffer( buffer->backend, (render_buffer_t*)buffer, true, true );
				memory_deallocate( buffer );
			}
			return;
		}
	} while( ref > 0 );
}


render_usage_t render_indexbuffer_usage( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? (render_usage_t)buffer->usage : RENDERUSAGE_INVALID;
}


unsigned int render_indexbuffer_num_allocated( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->allocated : 0;
}


unsigned int render_indexbuffer_num_elements( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->used : 0;
}


void render_indexbuffer_set_num_elements( object_t id, unsigned int num )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
	{
		buffer->used = ( buffer->allocated < num ) ? buffer->allocated : num;
		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}


void render_indexbuffer_lock( object_t id, unsigned int lock )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( render_indexbuffer_ref( id ) != id )
		return;
	if( lock & RENDERBUFFER_LOCK_WRITE )
	{
		atomic_incr32( &buffer->locks );
		buffer->access = buffer->store;
	}
	else if( lock & RENDERBUFFER_LOCK_READ )
	{
		atomic_incr32( &buffer->locks );
		buffer->access = buffer->store;
	}
	buffer->flags |= ( lock & RENDERBUFFER_LOCK_BITS );
}


void render_indexbuffer_unlock( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( !atomic_load32( &buffer->locks ) )
		return;
	if( atomic_decr32( &buffer->locks ) == 0 )
	{
		buffer->access = 0;
		if( ( buffer->flags & RENDERBUFFER_LOCK_WRITE ) && !( buffer->flags & RENDERBUFFER_LOCK_NOUPLOAD ) )
		{
			buffer->flags |= RENDERBUFFER_DIRTY;
			if( ( buffer->policy == RENDERBUFFER_UPLOAD_ONUNLOCK ) || ( buffer->flags & RENDERBUFFER_LOCK_FORCEUPLOAD ) )
				render_indexbuffer_upload( id );
		}
		buffer->flags &= ~RENDERBUFFER_LOCK_BITS;
	}
	render_indexbuffer_destroy( id );
}


render_buffer_uploadpolicy_t render_indexbuffer_upload_policy( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? (render_buffer_uploadpolicy_t)buffer->policy : RENDERBUFFER_UPLOAD_ONDISPATCH;
}


void render_indexbuffer_set_upload_policy( object_t id, render_buffer_uploadpolicy_t policy )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
		buffer->policy = policy;
}


void render_indexbuffer_upload( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer->flags & RENDERBUFFER_DIRTY )
		buffer->backend->vtable.upload_buffer( buffer->backend, (render_buffer_t*)buffer );
}


uint16_t* render_indexbuffer_element( object_t id, unsigned int element )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return pointer_offset( buffer->access, buffer->size * element );
}


unsigned int render_indexbuffer_element_size( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->size : 0;
}


void render_indexbuffer_release( object_t id, bool sys, bool aux )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
		buffer->backend->vtable.deallocate_buffer( buffer->backend, (render_buffer_t*)buffer, sys, aux );
}


void render_indexbuffer_restore( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
	{
		buffer->backend->vtable.allocate_buffer( buffer->backend, (render_buffer_t*)buffer );
		
		//...
		//All loadable resources should have a stream identifier, an offset and a size
		//to be able to repoen the stream and read the raw buffer back
		//...
		
		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}


uuid_t render_indexbuffer_uuid( object_t id )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->uuid : uuid_null();
}


void render_indexbuffer_set_uuid( object_t id, const uuid_t uuid )
{
	render_indexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
		buffer->uuid = uuid;
}
