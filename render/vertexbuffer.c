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


object_t render_vertexbuffer_create( render_backend_t* backend, render_usage_t usage, unsigned int vertices, const render_vertex_decl_t* decl, const void* data )
{
	object_t id = objectmap_reserve( _render_map_buffer );
	if( !id )
	{
		log_errorf( HASH_RENDER, ERROR_OUT_OF_MEMORY, "Unable to create vertex buffer, out of slots in object map" );
		return 0;
	}

	memory_context_push( HASH_RENDER );
	
	render_vertexbuffer_t* buffer = memory_allocate_zero( sizeof( render_vertexbuffer_t ), 0, MEMORY_PERSISTENT );
	buffer->id         = id;
	buffer->backend    = backend;
	buffer->usage      = usage;
	buffer->buffertype = RENDERBUFFER_VERTEX;
	buffer->policy     = RENDERBUFFER_UPLOAD_ONDISPATCH;
	buffer->size       = render_vertex_decl_size( decl );
	memcpy( &buffer->decl, decl, sizeof( render_vertex_decl_t ) );
	atomic_store32( &buffer->ref, 1 );
	objectmap_set( _render_map_buffer, id, buffer );
	
	if( vertices )
	{
		buffer->allocated = vertices;
		buffer->used = vertices;
		buffer->store = backend->vtable.allocate_buffer( backend, (render_buffer_t*)buffer );
		if( data )
		{
			memcpy( buffer->store, data, vertices * buffer->size );
			buffer->flags |= RENDERBUFFER_DIRTY;
		}
	}
	
	memory_context_pop();
	
	return buffer->id;
}


object_t render_vertexbuffer_ref( object_t id )
{
	int32_t ref;
	render_vertexbuffer_t* buffer = objectmap_lookup( _render_map_buffer, id );
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
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
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


render_usage_t render_vertexbuffer_usage( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? (render_usage_t)buffer->usage : RENDERUSAGE_INVALID;
}


const render_vertex_decl_t* render_vertexbuffer_decl( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? &buffer->decl : 0;
}


unsigned int render_vertexbuffer_num_allocated( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->allocated : 0;
}


unsigned int render_vertexbuffer_num_elements( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->used : 0;
}


void render_vertexbuffer_set_num_elements( object_t id, unsigned int num )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
	{
		buffer->used = ( buffer->allocated < num ) ? buffer->allocated : num;
		buffer->flags |= RENDERBUFFER_DIRTY;
	}
}


void render_vertexbuffer_lock( object_t id, unsigned int lock )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	if( render_vertexbuffer_ref( id ) != id )
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


void render_vertexbuffer_unlock( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
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


render_buffer_uploadpolicy_t render_vertexbuffer_upload_policy( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? (render_buffer_uploadpolicy_t)buffer->policy : RENDERBUFFER_UPLOAD_ONDISPATCH;
}


void render_vertexbuffer_set_upload_policy( object_t id, render_buffer_uploadpolicy_t policy )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
		buffer->policy = policy;
}


void render_verexbuffer_upload( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer->flags & RENDERBUFFER_DIRTY )
		buffer->backend->vtable.upload_buffer( buffer->backend, (render_buffer_t*)buffer );
}


void* render_vertexbuffer_element( object_t id, unsigned int element )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return pointer_offset( buffer->access, buffer->size * element );
}


unsigned int render_vertexbuffer_element_size( object_t id )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	return buffer ? buffer->size : 0;
}


void render_vertexbuffer_release( object_t id, bool sys, bool aux )
{
	render_vertexbuffer_t* buffer = GET_BUFFER( id );
	if( buffer )
		buffer->backend->vtable.deallocate_buffer( buffer->backend, (render_buffer_t*)buffer, sys, aux );
}


void render_vertexbuffer_restore( object_t id )
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


static const unsigned int _vertex_format_size[ VERTEXFORMAT_UNKNOWN+1 ] = {
	
	4,  //VERTEXFORMAT_FLOAT
	8,  //VERTEXFORMAT_FLOAT2
	12, //VERTEXFORMAT_FLOAT3
	16, //VERTEXFORMAT_FLOAT4
	
	4,  //VERTEXFORMAT_UBYTE4
	4,  //VERTEXFORMAT_UBYTE4_SNORM
	
	2,  //VERTEXFORMAT_SHORT
	4,  //VERTEXFORMAT_SHORT2
	8,  //VERTEXFORMAT_SHORT4,
	
	4,  //VERTEXFORMAT_INT
	8,  //VERTEXFORMAT_INT2
	16, //VERTEXFORMAT_INT4
	
	0,
	0
};

unsigned int render_vertex_decl_size( const render_vertex_decl_t* decl )
{
	unsigned int size = 0;
	for( int i = 0; i < VERTEXATTRIBUTE_NUMATTRIBUTES; ++i )
	{
		FOUNDATION_ASSERT_MSGFORMAT( decl->attribute[i].format <= VERTEXFORMAT_UNKNOWN, "Invalid vertex format type %d index %d", decl->attribute[i].format, i );
		unsigned int end = decl->attribute[i].offset + _vertex_format_size[ decl->attribute[i].format ];
		if( end > size )
			size = end;
	}
	return size;
}


render_vertex_decl_t* render_vertex_decl_allocate_buffer( unsigned int num_elements, render_vertex_decl_element_t* elements )
{
	render_vertex_decl_t* decl = memory_allocate_zero_context( HASH_RENDER, sizeof( render_vertex_decl_t ), 0, MEMORY_PERSISTENT );
	for( int i = 0; i < VERTEXATTRIBUTE_NUMATTRIBUTES; ++i )
		decl->attribute[i].format = VERTEXFORMAT_UNKNOWN;
	
	if( num_elements > VERTEXATTRIBUTE_NUMATTRIBUTES )
		num_elements = VERTEXATTRIBUTE_NUMATTRIBUTES;
	
	uint16_t offset = 0;
	for( unsigned int i = 0; i < num_elements; ++i )
	{
		decl->attribute[elements[i].attribute].format = elements[i].format;
		decl->attribute[elements[i].attribute].binding = 0;
		decl->attribute[elements[i].attribute].offset = offset;
		
		offset += _vertex_format_size[ elements[i].format ];
	}
	
	return decl;
}


render_vertex_decl_t* render_vertex_decl_allocate( render_vertex_format_t format, render_vertex_attribute_id attribute, ... )
{
	render_vertex_decl_t* decl = memory_allocate_zero_context( HASH_RENDER, sizeof( render_vertex_decl_t ), 0, MEMORY_PERSISTENT );
	for( int i = 0; i < VERTEXATTRIBUTE_NUMATTRIBUTES; ++i )
		decl->attribute[i].format = VERTEXFORMAT_UNKNOWN;
	
	uint16_t offset = 0;
	
	va_list list;
	va_start( list, attribute );
	
	while( format < VERTEXFORMAT_UNKNOWN )
	{
		if( attribute < VERTEXATTRIBUTE_NUMATTRIBUTES )
		{
			decl->attribute[attribute].format = format;
			decl->attribute[attribute].binding = 0;
			decl->attribute[attribute].offset = offset;
			
			offset += _vertex_format_size[ format ];
		}
		
		format = va_arg( list, render_vertex_format_t );
		if( format <= VERTEXFORMAT_UNKNOWN )
			attribute = va_arg( list, render_vertex_attribute_id );
	}
	
	va_end( list );
	
	return decl;
}

