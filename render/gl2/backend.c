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
#include <window/window.h>
#include <resource/hashstrings.h>
#include <render/render.h>
#include <render/internal.h>

#include <render/gl2/backend.h>

#if FOUNDATION_PLATFORM_WINDOWS || FOUNDATION_PLATFORM_MACOS || ( FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI )

#include <render/gl4/glwrap.h>

#define GET_BUFFER(id) objectmap_lookup(_render_map_buffer, (id))

#if RENDER_ENABLE_NVGLEXPERT
#  include <nvapi.h>

static void
nvoglexpert_callback(unsigned int category, unsigned int id, unsigned int detail, int object,
                     const char* msg) {
	log_warn(HASH_RENDER,
	         STRING_CONST("nVidia OpenGL Expert error: Category 0x%08x, Message 0x%08x : %s"), category, id,
	         msg);
}

#endif

#include <render/gl4/glprocs.h>

typedef struct render_backend_gl2_t {
	RENDER_DECLARE_BACKEND;

	void* context;
#if FOUNDATION_PLATFORM_WINDOWS
	HDC hdc;
	DWORD thread_hglrc;
#endif

	render_resolution_t resolution;

	bool use_clear_scissor;
} render_backend_gl2_t;

static void
_rb_gl2_set_default_state(void);

static bool
_rb_gl2_construct(render_backend_t* backend) {

	//TODO: Caps check
	//if( !... )
	//  return false;

#if FOUNDATION_PLATFORM_WINDOWS
	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;
	backend_gl2->thread_hglrc = TlsAlloc();
#else
	FOUNDATION_UNUSED(backend);
#endif

	log_debug(HASH_RENDER, STRING_CONST("Constructed GL2 render backend"));
	return true;
}

static void
_rb_gl2_destruct(render_backend_t* backend) {
	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;

#if FOUNDATION_PLATFORM_WINDOWS
	if (backend_gl2->thread_hglrc) {
		void* hglrc = TlsGetValue(backend_gl2->thread_hglrc);
		if (hglrc && (hglrc != backend_gl2->context))
			_rb_gl_destroy_context(backend_gl2->drawable, hglrc);
		TlsFree(backend_gl2->thread_hglrc);
		backend_gl2->thread_hglrc = 0;
	}
#endif

	if (backend_gl2->context)
		_rb_gl_destroy_context(backend_gl2->drawable, backend_gl2->context);
	backend_gl2->context = 0;

	log_debug(HASH_RENDER, STRING_CONST("Destructed GL2 render backend"));
}

static bool
_rb_gl2_set_drawable(render_backend_t* backend, render_drawable_t* drawable) {
	if (!FOUNDATION_VALIDATE_MSG(drawable->type != RENDERDRAWABLE_OFFSCREEN,
	                             "Offscreen drawable not implemented"))
		return error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);

	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;
	if (!FOUNDATION_VALIDATE_MSG(!backend_gl2->context, "Drawable switching not supported yet"))
		return error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);

	backend_gl2->context = _rb_gl_create_context(drawable, 2, 0, 0);
	if (!backend_gl2->context) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to create OpenGL 2 context"));
		return false;
	}

#if FOUNDATION_PLATFORM_WINDOWS
	backend_gl2->hdc = (HDC)drawable->hdc;
	TlsSetValue(backend_gl2->thread_hglrc, backend_gl2->context);
#endif

#if FOUNDATION_PLATFORM_LINUX
	glXMakeCurrent(drawable->display, (GLXDrawable)drawable->drawable, backend_gl2->context);
	if (True == glXIsDirect(drawable->display, backend_gl2->context))
		log_debug(HASH_RENDER, STRING_CONST("Direct rendering enabled"));
	else
		log_warn(HASH_RENDER, WARNING_PERFORMANCE, STRING_CONST("Indirect rendering"));
#endif

#if RENDER_ENABLE_NVGLEXPERT
	NvAPI_OGL_ExpertModeSet(20, NVAPI_OGLEXPERT_REPORT_ALL, NVAPI_OGLEXPERT_OUTPUT_TO_CALLBACK,
	                        nvoglexpert_callback);
#endif

#if BUILD_ENABLE_LOG
	const char* vendor   = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version  = (const char*)glGetString(GL_VERSION);
	const char* ext      = (const char*)glGetString(GL_EXTENSIONS);
	glGetError();

	log_infof(HASH_RENDER, STRING_CONST("Vendor:     %s"), vendor ? vendor : "<unknown>");
	log_infof(HASH_RENDER, STRING_CONST("Renderer:   %s"), renderer ? renderer : "<unknown>");
	log_infof(HASH_RENDER, STRING_CONST("Version:    %s"), version ? version : "<unknown>");
	log_infof(HASH_RENDER, STRING_CONST("Extensions: %s"), ext ? ext : "<none>");
#endif

#if FOUNDATION_PLATFORM_WINDOWS
	const char* wglext = 0;
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = 0;
	if ((wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
	                                 _rb_gl_get_proc_address("wglGetExtensionsStringARB")) != 0)
		wglext = wglGetExtensionsStringARB((HDC)drawable->hdc);
	else if ((wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
	                                      _rb_gl_get_proc_address("wglGetExtensionsStringEXT")) != 0)
		wglext = wglGetExtensionsStringEXT();
	log_debugf(HASH_RENDER, STRING_CONST("WGL Extensions: %s"), wglext ? wglext : "<none>");
#endif

	if (!_rb_gl_get_standard_procs(2, 0))
		return false;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);

	glViewport(0, 0, render_drawable_width(drawable), render_drawable_height(drawable));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glCullFace(GL_BACK);

	_rb_gl2_set_default_state();

	_rb_gl_check_error("Error setting up default state");

	return true;
}

static void
_rb_gl2_enable_thread(render_backend_t* backend) {
	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;

#if FOUNDATION_PLATFORM_WINDOWS
	void* thread_context = TlsGetValue(backend_gl2->thread_hglrc);
	if (!thread_context) {
		thread_context = _rb_gl_create_context(backend->drawable, 2, 0, backend_gl2->context);
		TlsSetValue(backend_gl2->thread_hglrc, thread_context);
	}
	if (!wglMakeCurrent((HDC)backend->drawable->hdc, (HGLRC)thread_context)) {
		string_const_t errmsg = system_error_message(0);
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to enable thread for GL2 rendering: %.*s"),
		           STRING_FORMAT(errmsg));
	}
#elif FOUNDATION_PLATFORM_LINUX
	glXMakeCurrent(backend->drawable->display, (GLXDrawable)backend->drawable->drawable,
	               backend_gl2->context);
	_rb_gl_check_error("Unable to enable thread for rendering");
#elif FOUNDATION_PLATFORM_MACOS
	_rb_gl_make_agl_context_current(backend_gl2->context);
#else
	FOUNDATION_ASSERT_FAIL("Unable to enable thread for rendering, platform not implemented");
	error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
#endif
}

static void
_rb_gl2_disable_thread(render_backend_t* backend) {
#if FOUNDATION_PLATFORM_WINDOWS
	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;
	void* thread_context = TlsGetValue(backend_gl2->thread_hglrc);
	if (thread_context && (thread_context != backend_gl2->context)) {
		_rb_gl_destroy_context(backend_gl2->drawable, thread_context);
		TlsSetValue(backend_gl2->thread_hglrc, 0);
	}
#else
	FOUNDATION_UNUSED(backend);
#endif
}

static void*
_rb_gl2_allocate_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	return memory_allocate(HASH_RENDER, buffer->size * buffer->allocated, 16, MEMORY_PERSISTENT);
}

static void
_rb_gl2_deallocate_buffer(render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux) {
	FOUNDATION_UNUSED(backend);
	if (sys)
		memory_deallocate(buffer->store);

	if (aux && buffer->backend_data[0]) {
		GLuint buffer_object = (GLuint)buffer->backend_data[0];
		glDeleteBuffers(1, &buffer_object);
		buffer->backend_data[0] = 0;
	}
}

static bool
_rb_gl2_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	if ((buffer->buffertype == RENDERBUFFER_PARAMETER) || (buffer->buffertype == RENDERBUFFER_STATE))
		return true;

	GLuint buffer_object = (GLuint)buffer->backend_data[0];
	if (!buffer_object) {
		glGenBuffers(1, &buffer_object);
		if (_rb_gl_check_error("Unable to create buffer object"))
			return false;
		buffer->backend_data[0] = buffer_object;
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer_object);
	glBufferData(GL_ARRAY_BUFFER, (long)(buffer->size * buffer->allocated), buffer->store,
	             (buffer->usage == RENDERUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	if (_rb_gl_check_error("Unable to upload buffer object data"))
		return false;

	buffer->flags &= ~(uint32_t)RENDERBUFFER_DIRTY;
	return true;
}

static void
_rb_gl2_link_buffer(render_backend_t* backend, render_buffer_t* buffer, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(program);
}

static bool
_rb_gl2_upload_shader(render_backend_t* backend, render_shader_t* shader, const void* buffer,
                      size_t size) {
	bool is_pixel_shader = (shader->shadertype == SHADER_PIXEL);
	//render_backend_gl2_t* backend_gl2 = (render_backend_gl4_t*)backend;
	FOUNDATION_UNUSED(backend);

	//Shader backend data:
	//  0 - Shader object
	if (shader->backend_data[0])
		glDeleteShader((GLuint)shader->backend_data[0]);

	GLuint handle;
	const GLchar* source = (const GLchar*)buffer;
	GLint source_size = (GLint)size;
	GLint compiled = 0;

	switch (shader->shadertype) {
	case SHADER_PIXEL:
	case SHADER_VERTEX:
		handle = glCreateShader(is_pixel_shader ? GL_FRAGMENT_SHADER_ARB : GL_VERTEX_SHADER_ARB);
		if (!handle) {
			if (!_rb_gl_check_error("Unable to compile shader: Error creating shader"))
				log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL,
				           STRING_CONST("Unable to compile shader: Error creating shader (no error)"));
			break;
		}

		glShaderSource(handle, 1, &source, &source_size);
		glCompileShader(handle);
		glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);

		if (!compiled) {
#if BUILD_DEBUG
			GLsizei log_capacity = 2048;
			GLchar* log_buffer = memory_allocate(HASH_RESOURCE, (size_t)log_capacity, 0, MEMORY_TEMPORARY);
			GLint log_length = 0;
			glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);
			glGetShaderInfoLog(handle, log_capacity, &log_length, log_buffer);
			log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to compile shader: %.*s (handle %u)"),
			           (int)log_length, log_buffer, (unsigned int)handle);
			memory_deallocate(log_buffer);
#endif
			glDeleteShader(handle);
			shader->backend_data[0] = 0;
		}
		else {
			shader->backend_data[0] = handle;
		}
		break;

	default:
		break;
	}
	return (compiled != 0);
}

static void
_rb_gl2_deallocate_shader(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend);
	if (shader->backend_data[0])
		glDeleteShader((GLuint)shader->backend_data[0]);
	shader->backend_data[0] = 0;
}

static bool
_rb_gl2_check_program_link(GLuint handle) {
	GLint result = 0;
	glGetProgramiv(handle, GL_LINK_STATUS, &result);
	if (!result) {
		GLsizei buffer_size = 4096;
		GLint log_length = 0;
		GLchar* log = 0;

		glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &buffer_size);
		log = memory_allocate(HASH_RENDER, (size_t)buffer_size + 1, 0, MEMORY_TEMPORARY);
		glGetProgramInfoLog(handle, buffer_size, &log_length, log);

		log_errorf(ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to compile program: %.*s"),
		           (int)log_length, log);
		memory_deallocate(log);

		glDeleteProgram(handle);

		return false;
	}
	return true;
}

static bool
_rb_gl2_upload_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	if (program->backend_data[0])
		glDeleteProgram((GLuint)program->backend_data[0]);

	GLint attributes = 0;
	GLint uniforms = 0;
	GLint ia, iu;
	GLsizei num_chars = 0;
	GLint size = 0;
	GLenum type = GL_NONE;
	hash_t name_hash;
	char name[256];
	GLuint handle = glCreateProgram();

	render_shader_t* vshader = program->vertexshader;
	render_shader_t* pshader = program->pixelshader;
	if (!vshader || !pshader)
		return false;

	glAttachShader(handle, (GLuint)vshader->backend_data[0]);
	glAttachShader(handle, (GLuint)pshader->backend_data[0]);
	glLinkProgram(handle);
	if (!_rb_gl2_check_program_link(handle))
		return false;

	//TODO: By storing name strings in render_program_t we can avoid double link
	//      by doing all glBindAttribLocation after attach but before link
	glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &attributes);
	for (ia = 0; ia < attributes; ++ia) {
		num_chars = 0;
		size = 0;
		type = GL_NONE;
		glGetActiveAttrib(handle, (GLuint)ia, sizeof(name), &num_chars, &size, &type, name);

		name_hash = hash(name, (size_t)num_chars);
		for (size_t iattrib = 0; iattrib < program->attributes.num_attributes; ++iattrib) {
			if (program->attribute_name[iattrib] == name_hash) {
				render_vertex_attribute_t* attribute = program->attributes.attribute + iattrib;
				glBindAttribLocation(handle, attribute->binding, name);
				break;
			}
		}
	}
	glLinkProgram(handle);
	if (!_rb_gl2_check_program_link(handle))
		return false;

	glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);
	for (iu = 0; iu < uniforms; ++iu) {
		num_chars = 0;
		size = 0;
		type = GL_NONE;
		glGetActiveUniform(handle, (GLuint)iu, sizeof(name), &num_chars, &size, &type, name);

		name_hash = hash(name, (size_t)num_chars);
		for (size_t iparam = 0; iparam < program->num_parameters; ++iparam) {
			render_parameter_t* parameter = program->parameters + iparam;
			if (parameter->name == name_hash)
				parameter->location = (unsigned int)glGetUniformLocation(handle, name);
		}
	}

	program->backend_data[0] = handle;

	return true;
}

static void
_rb_gl2_deallocate_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	if (program->backend_data[0])
		glDeleteProgram((GLuint)program->backend_data[0]);
	program->backend_data[0] = 0;
}

#if 0
static render_parameter_block_t* _rb_gl2_allocate_parameter_block(render_backend_t* backend,
        const render_parameter_t* parameters, unsigned int num) {
	render_parameter_block_t* block = memory_allocate(HASH_RENDER,
	                                                  sizeof(render_parameter_block_t) + (sizeof(render_parameter_info_t) * num), 0,
	                                                  MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZE);
	for (unsigned int i = 0; i < num; ++i) {
		block->info[i].type   = parameters[i].type;
		block->info[i].dim    = parameters[i].dim;
		block->info[i].offset = 0;
	}
	return block;
}


static void _rb_gl2_bind_parameter_block(render_backend_t* backend, render_parameter_block_t* block,
                                         unsigned int index, const void* RESTRICT data) {
}


static void _rb_gl2_deallocate_parameter_block(render_backend_t* backend,
                                               render_parameter_block_t* block) {
	memory_deallocate(block);
}


static void _rb_gl2_upload_texture(render_backend_t* backend, render_texture_t* texture,
                                   unsigned int width, unsigned int height, unsigned int depth, unsigned int levels,
                                   const void* data) {
}


static render_texture_t* _rb_gl2_allocate_texture(render_backend_t* backend,
                                                  render_texture_type_t type, render_usage_t usage, pixelformat_t format, colorspace_t colorspace) {
	render_texture_t* texture = memory_allocate(HASH_RENDER, sizeof(render_texture_t), 0,
	                                            MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZE);
	texture->textype = type;
	texture->usage = usage;
	texture->format = format;
	texture->colorspace = colorspace;
	return texture;
}


static void _rb_gl2_deallocate_texture(render_backend_t* backend, render_texture_t* texture) {
	memory_deallocate(texture);
}
#endif

static void
_rb_gl2_clear(render_backend_gl2_t* backend, render_context_t* context, render_command_t* command) {
	FOUNDATION_UNUSED(context);
	unsigned int buffer_mask = command->data.clear.buffer_mask;
	unsigned int bits = 0;

	if (buffer_mask & RENDERBUFFER_COLOR) {
		unsigned int color_mask = command->data.clear.color_mask;
		uint32_t color = command->data.clear.color;
		glColorMask((color_mask & 0x01) ? GL_TRUE : GL_FALSE, (color_mask & 0x02) ? GL_TRUE : GL_FALSE,
		            (color_mask & 0x04) ? GL_TRUE : GL_FALSE, (color_mask & 0x08) ? GL_TRUE : GL_FALSE);
		bits |= GL_COLOR_BUFFER_BIT;
		//color_linear_t color = uint32_to_color( command->data.clear.color );
		//glClearColor( vector_x( color ), vector_y( color ), vector_z( color ), vector_w( color ) );
		glClearColor((float)(color & 0xFF) / 255.0f, (float)((color >> 8) & 0xFF) / 255.0f,
		             (float)((color >> 16) & 0xFF) / 255.0f, (float)((color >> 24) & 0xFF) / 255.0f);
	}

	if (buffer_mask & RENDERBUFFER_DEPTH) {
		glDepthMask(GL_TRUE);
		bits |= GL_DEPTH_BUFFER_BIT;
		glClearDepth((GLclampd)command->data.clear.depth);
	}

	if (buffer_mask & RENDERBUFFER_STENCIL) {
		glClearStencil((GLint)command->data.clear.stencil);
		bits |= GL_STENCIL_BUFFER_BIT;
	}

	if (backend->use_clear_scissor)
		glEnable(GL_SCISSOR_TEST);

	glClear(bits);

	if (backend->use_clear_scissor)
		glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

static void
_rb_gl2_viewport(render_backend_gl2_t* backend, render_context_t* context,
                 render_command_t* command) {
	int target_width = render_target_width(context->target);
	int target_height = render_target_height(context->target);

	GLint x = command->data.viewport.x;
	GLint y = command->data.viewport.y;
	GLsizei w = command->data.viewport.width;
	GLsizei h = command->data.viewport.height;

	glViewport(x, y, w, h);
	glScissor(x, y, w, h);

	backend->use_clear_scissor = (x || y || (w != target_width) || (h != target_height));
}

static const GLint        _rb_gl2_vertex_format_size[VERTEXFORMAT_NUMTYPES] = { 1,        2,        3,        4,        4,                4,                1,        2,        4,        1,        2,        4        };
static const GLenum       _rb_gl2_vertex_format_type[VERTEXFORMAT_NUMTYPES] = { GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_UNSIGNED_BYTE, GL_BYTE,          GL_SHORT, GL_SHORT, GL_SHORT, GL_INT,   GL_INT,   GL_INT   };
static const GLboolean    _rb_gl2_vertex_format_norm[VERTEXFORMAT_NUMTYPES] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE,          GL_TRUE,          GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };

static const GLenum       _rb_gl2_primitive_type[RENDERPRIMITIVE_NUMTYPES] = { GL_TRIANGLES };
static const unsigned int _rb_gl2_primitive_mult[RENDERPRIMITIVE_NUMTYPES] = { 3 };
static const unsigned int _rb_gl2_primitive_add[RENDERPRIMITIVE_NUMTYPES]  = { 0 };

//                                                 BLEND_ZERO, BLEND_ONE, BLEND_SRCCOLOR, BLEND_INVSRCCOLOR,      BLEND_DESTCOLOR, BLEND_INVDESTCOLOR,     BLEND_SRCALPHA, BLEND_INVSRCALPHA,      BLEND_DESTALPHA, BLEND_INVDESTALPHA,     BLEND_FACTOR,      BLEND_INVFACTOR,             BLEND_SRCALPHASAT
//static const GLenum       _rb_gl2_blend_func[] = { GL_ZERO,    GL_ONE,    GL_SRC_COLOR,   GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR,    GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,   GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA,    GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE };

static void
_rb_gl2_set_default_state(void) {
	glBlendFunc(GL_ONE, GL_ZERO);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
}

static void
_rb_gl2_set_state(render_state_t* state) {
	FOUNDATION_UNUSED(state);
}

static void
_rb_gl2_render(render_backend_gl2_t* backend, render_context_t* context,
               render_command_t* command) {
	render_vertexbuffer_t* vertexbuffer = GET_BUFFER(command->data.render.vertexbuffer);
	render_indexbuffer_t* indexbuffer  = GET_BUFFER(command->data.render.indexbuffer);
	render_parameterbuffer_t* parameterbuffer = GET_BUFFER(command->data.render.parameterbuffer);
	render_program_t* program = command->data.render.program;
	FOUNDATION_UNUSED(context);

	if (!vertexbuffer || !indexbuffer || !parameterbuffer || !program) { //Outdated references
		FOUNDATION_ASSERT_FAIL("Render command using invalid resources");
		return;
	}

	if (vertexbuffer->flags & RENDERBUFFER_DIRTY)
		_rb_gl2_upload_buffer((render_backend_t*)backend, (render_buffer_t*)vertexbuffer);
	if (indexbuffer->flags & RENDERBUFFER_DIRTY)
		_rb_gl2_upload_buffer((render_backend_t*)backend, (render_buffer_t*)indexbuffer);

	//Bind vertex attributes
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vertexbuffer->backend_data[0]);

	const render_vertex_decl_t* decl = &vertexbuffer->decl;
	for (unsigned int attrib = 0; attrib < VERTEXATTRIBUTE_NUMATTRIBUTES; ++attrib) {
		const uint8_t format = decl->attribute[attrib].format;
		if (format < VERTEXFORMAT_NUMTYPES) {
			glVertexAttribPointer(attrib, _rb_gl2_vertex_format_size[format],
			                      _rb_gl2_vertex_format_type[format], _rb_gl2_vertex_format_norm[format],
			                      (GLsizei)vertexbuffer->size,
			                      (const void*)(uintptr_t)decl->attribute[attrib].offset);
			glEnableVertexAttribArray(attrib);
		}
		else {
			glDisableVertexAttribArray(attrib);
		}
	}

	//Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)indexbuffer->backend_data[0]);

	//Bind programs/shaders
	glUseProgram((GLuint)program->backend_data[0]);

	// Bind the parameter blocks
	render_parameter_t* param = parameterbuffer->parameters;
	for (unsigned int ip = 0; ip < parameterbuffer->num_parameters; ++ip, ++param) {
		/*if (param->type == RENDERPARAMETER_TEXTURE) {
			//TODO: Dynamic use of texture units, reusing unit that already have correct texture bound, and least-recently-used evicting old bindings to free a new unit
			glActiveTexture(GL_TEXTURE0 + param_info->unit);
			glEnable(GL_TEXTURE_2D);

			object_t object = *(object_t*)pointer_offset(block, param_info->offset);
			render_texture_gl2_t* texture = object ? pool_lookup(_global_pool_texture, object) : 0;
			NEO_ASSERT_MSGFORMAT(!object ||
			                     texture, "Parameter block using old/invalid texture 0x%llx", object);

			glBindTexture(GL_TEXTURE_2D, texture ? texture->object : 0);

			for (unsigned int iu = 0; iu < program->num_uniforms; ++iu) {
				if (program->uniforms[iu].name == *param_name) {
					glUniform1i(program->uniforms[iu].location, param_info->unit);
					break;
				}
			}
		}
		else*/ {
			void* data = pointer_offset(parameterbuffer->store, param->offset);
			if (param->type == RENDERPARAMETER_FLOAT4)
				glUniform4fv((GLint)param->location, param->dim, data);
			else if (param->type == RENDERPARAMETER_INT4)
				glUniform4iv((GLint)param->location, param->dim, data);
			else if (param->type == RENDERPARAMETER_MATRIX)
				glUniformMatrix4fv((GLint)param->location, param->dim, GL_TRUE, data);
		}
	}

	//TODO: Proper states
	/*ID3D10Device_RSSetState( device, backend_dx10->rasterizer_state[0].state );

	FLOAT blend_factors[] = { 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f };
	ID3D10Device_OMSetBlendState( device, backend_dx10->blend_state[ ( command->data.render.blend_state >> 48ULL ) & 0xFFFFULL ].state, blend_factors, 0xFFFFFFFF );
	ID3D10Device_OMSetDepthStencilState( device, backend_dx10->depthstencil_state[0].state, 0xFFFFFFFF );*/

	if (command->data.render.statebuffer) {
		//Set state from buffer
		render_statebuffer_t* buffer = GET_BUFFER(command->data.render.statebuffer);
		_rb_gl2_set_state(&buffer->state);
	}
	else {
		//Set default state
		_rb_gl2_set_default_state();
	}

	unsigned int primitive = command->type - RENDERCOMMAND_RENDER_TRIANGLELIST;
	unsigned int num = command->count;
	unsigned int pnum = _rb_gl2_primitive_mult[primitive] * num + _rb_gl2_primitive_add[primitive];

	glDrawElements(_rb_gl2_primitive_type[primitive], (GLsizei)pnum,
	               (indexbuffer->size == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 0);
}

static void
_rb_gl2_dispatch(render_backend_t* backend, render_context_t** contexts, size_t num_contexts) {
	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;

	for (size_t context_index = 0, context_size = num_contexts; context_index < context_size;
	        ++context_index) {
		render_context_t* context = contexts[context_index];
		render_command_t* command = context->commands;
		const radixsort_index_t* order = context->order;

		int cmd_size = atomic_load32(&context->reserved, memory_order_acquire);
		for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order) {
			command = context->commands + *order;
			switch (command->type) {
			case RENDERCOMMAND_CLEAR:
				_rb_gl2_clear(backend_gl2, context, command);
				break;

			case RENDERCOMMAND_VIEWPORT:
				_rb_gl2_viewport(backend_gl2, context, command);
				break;

			case RENDERCOMMAND_RENDER_TRIANGLELIST:
				_rb_gl2_render(backend_gl2, context, command);
				break;
			}
		}
	}
}

static void
_rb_gl2_flip(render_backend_t* backend) {
	render_backend_gl2_t* backend_gl2 = (render_backend_gl2_t*)backend;

#if FOUNDATION_PLATFORM_WINDOWS

	if (backend_gl2->hdc) {
		if (!SwapBuffers(backend_gl2->hdc)) {
			string_const_t errmsg = system_error_message(0);
			log_warnf(HASH_RENDER, WARNING_SYSTEM_CALL_FAIL, STRING_CONST("SwapBuffers failed: %.*s"),
			          STRING_FORMAT(errmsg));
		}
	}

#elif FOUNDATION_PLATFORM_MACOS

	if (backend_gl2->context) {
		/*if( backend_gl2->fullscreen )
			CGLFlushDrawable( backend_gl2->context );
		else*/
		_rb_gl_flush_drawable(backend_gl2->context);
	}

#elif FOUNDATION_PLATFORM_LINUX

	if (backend_gl2->drawable->display)
		glXSwapBuffers(backend_gl2->drawable->display, (GLXDrawable)backend_gl2->drawable->drawable);

#else
#  error Not implemented
#endif

	++backend->framecount;
}

static render_backend_vtable_t _render_backend_vtable_gl2 = {
	.construct = _rb_gl2_construct,
	.destruct = _rb_gl2_destruct,
	.enumerate_adapters = _rb_gl_enumerate_adapters,
	.enumerate_modes = _rb_gl_enumerate_modes,
	.set_drawable = _rb_gl2_set_drawable,
	.enable_thread = _rb_gl2_enable_thread,
	.disable_thread = _rb_gl2_disable_thread,
	.allocate_buffer = _rb_gl2_allocate_buffer,
	.upload_buffer = _rb_gl2_upload_buffer,
	.upload_shader = _rb_gl2_upload_shader,
	.upload_program = _rb_gl2_upload_program,
	.link_buffer = _rb_gl2_link_buffer,
	.deallocate_buffer = _rb_gl2_deallocate_buffer,
	.deallocate_shader = _rb_gl2_deallocate_shader,
	.deallocate_program = _rb_gl2_deallocate_program,
	/*
	.allocate_texture = _rb_gl2_allocate_texture,
	.upload_texture = _rb_gl2_upload_texture,
	.deallocate_texture = _rb_gl2_deallocate_texture,
	*/
	.dispatch = _rb_gl2_dispatch,
	.flip = _rb_gl2_flip
};

render_backend_t*
render_backend_gl2_allocate(void) {
	if (!_rb_gl_check_context(2, 0))
		return 0;

#if RENDER_ENABLE_NVGLEXPERT
	static bool nvInitialized = false;
	if (!nvInitialized) {
		nvInitialized = true;
		NvAPI_Initialize();
	}
#endif

	render_backend_gl2_t* backend = memory_allocate(HASH_RENDER, sizeof(render_backend_gl2_t), 0,
	                                                MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_OPENGL2;
	backend->api_group = RENDERAPIGROUP_OPENGL;
	backend->vtable = _render_backend_vtable_gl2;
	return (render_backend_t*)backend;
}

#else

render_backend_t*
render_gl2_allocate(void) {
	return 0;
}

#endif
