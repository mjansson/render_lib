/* command.h  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
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

/*! \file command.h
    Render command */

#include <foundation/platform.h>

#include <render/types.h>

RENDER_API render_command_t*
render_command_allocate(void);

RENDER_API void
render_command_null(render_command_t* command);

RENDER_API void
render_command_clear(render_command_t* command, unsigned int buffer_mask, uint32_t color,
                     unsigned int color_mask, float depth, uint32_t stencil);

RENDER_API void
render_command_viewport(render_command_t* command, unsigned int x, unsigned int y,
                        unsigned int width, unsigned int height, real min_z, real max_z);

RENDER_API void
render_command_render(render_command_t* command, render_primitive_t type, uint16_t num,
                      render_program_t* program, object_t vertexbuffer,
                      object_t indexbuffer, object_t parameterbuffer,
                      object_t statebuffer);

