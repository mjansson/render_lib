/* command.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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


render_command_t* render_command_allocate( void )
{
	return memory_allocate_context( HASH_RENDER, sizeof( render_command_t ), 0, MEMORY_PERSISTENT );
}


void render_command_null( render_command_t* command )
{
	command->type = RENDERCOMMAND_INVALID;
}


void render_command_clear( render_command_t* command, unsigned int buffer_mask, uint32_t color, unsigned int color_mask, uint32_t depth, uint32_t stencil )
{
	command->type                   = RENDERCOMMAND_CLEAR;
	command->data.clear.buffer_mask = buffer_mask;
	command->data.clear.color       = color;
	command->data.clear.color_mask  = color_mask;
	command->data.clear.depth       = depth;
	command->data.clear.stencil     = stencil;
}


void render_command_viewport( render_command_t* command, unsigned int x, unsigned int y, unsigned int width, unsigned int height, real min_z, real max_z )
{
	command->type                   = RENDERCOMMAND_VIEWPORT;
	command->data.viewport.x        = (uint16_t)x;
	command->data.viewport.y        = (uint16_t)y;
	command->data.viewport.width    = (uint16_t)width;
	command->data.viewport.height   = (uint16_t)height;
	command->data.viewport.min_z    = min_z;
	command->data.viewport.max_z    = max_z;
}


void render_command_render( render_command_t* command, render_primitive_t type, uint16_t num, object_t vertexshader, object_t pixelshader, object_t vertexbuffer, object_t indexbuffer, object_t parameter_block, uint64_t blend_state )
{
	command->type                         = RENDERCOMMAND_RENDER_TRIANGLELIST + type;
	command->count                        = num;
	command->data.render.vertexshader     = vertexshader;
	command->data.render.pixelshader      = pixelshader;
	command->data.render.vertexbuffer     = vertexbuffer;
	command->data.render.indexbuffer      = indexbuffer;
	command->data.render.parameterblock   = parameter_block;
	command->data.render.blend_state      = blend_state;
	
#if FOUNDATION_BUILD_DEBUG
	/*void* vbuffer = pool_lookup( _global_pool_renderbuffer, vertexbuffer );
	void* ibuffer  = pool_lookup( _global_pool_renderbuffer, indexbuffer );
	void* vshader = pool_lookup( _global_pool_shader, vertexshader );
	void* pshader  = pool_lookup( _global_pool_shader, pixelshader );
	if( !FOUNDATION_VALIDATE_MSG( vbuffer && ibuffer && vshader && pshader, "Render command using invalid resources" ) )
		render_command_null( command );*/
#endif
}
