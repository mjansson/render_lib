/* backend.h  -  Render library  -  Public Domain  -  2022 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

/*! \file dx12/backend.h
    DirectX 12 render backend */

#include <foundation/platform.h>
#include <render/types.h>

RENDER_API render_backend_t*
render_backend_directx12_allocate(void);
