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
#include <render/render.h>
#include <render/internal.h>

#include <render/gles2/backend.h>

#if FOUNDATION_PLATFORM_IOS

#  include <OpenGLES/ES2/gl.h>
#  include <OpenGLES/ES2/glext.h>

//Objective-C interface
RENDER_EXTERN const void*
_rb_gles2_ios_create_egl_context(void);

RENDER_EXTERN void
_rb_gles2_ios_set_current_egl_context(const void* context);

RENDER_EXTERN void
_rb_gles2_ios_destroy_egl_context(const void* context);

RENDER_EXTERN int
_rb_gles2_ios_screen_width(void);

RENDER_EXTERN int
_rb_gles2_ios_screen_height(void);

RENDER_EXTERN bool
_rb_gles2_ios_render_buffer_storage_from_drawable(const void* context, const void* drawable,
                                                  unsigned int framebuffer, unsigned int colorbuffer);

RENDER_EXTERN void
_rb_gles2_ios_present_render_buffer(const void* context);

#endif

#if FOUNDATION_PLATFORM_IOS || FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

typedef struct render_backend_gles2_t {
	RENDER_DECLARE_BACKEND;

	object_t    render_thread;

#if FOUNDATION_PLATFORM_IOS
	const void* context;
	GLuint      framebuffer_id;
	GLuint      renderbuffer_id;
	GLuint      depthbuffer_id;
#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	EGLDisplay* display;
	EGLConfig   config;
	EGLSurface  surface;
	EGLContext  context;
#endif

	int         framebuffer_width;
	int         framebuffer_height;

	bool        use_clear_scissor;

} render_backend_gles2_t;

#if FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

const char*
_rb_gles2_egl_error_message(EGLint err) {
	switch (err) {
	case EGL_NOT_INITIALIZED:
		return "EGL_NOT_INITIALIZED";
	case EGL_BAD_ACCESS:
		return "EGL_BAD_ACCESS";
	case EGL_BAD_ALLOC:
		return "EGL_BAD_ALLOC";
	case EGL_BAD_ATTRIBUTE:
		return "EGL_BAD_ATTRIBUTE";
	case EGL_BAD_CONFIG:
		return "EGL_BAD_CONFIG";
	case EGL_BAD_CONTEXT:
		return "EGL_BAD_CONTEXT";
	case EGL_BAD_CURRENT_SURFACE:
		return "EGL_BAD_CURRENT_SURFACE";
	case EGL_BAD_DISPLAY:
		return "EGL_BAD_DISPLAY";
	case EGL_BAD_MATCH:
		return "EGL_BAD_MATCH";
	case EGL_BAD_NATIVE_PIXMAP:
		return "EGL_BAD_NATIVE_PIXMAP";
	case EGL_BAD_NATIVE_WINDOW:
		return "EGL_BAD_NATIVE_WINDOW";
	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";
	case EGL_BAD_SURFACE:
		return "EGL_BAD_SURFACE";
	case EGL_CONTEXT_LOST:
		return "EGL_CONTEXT_LOST";
	default:
		break;
	}
	return "<UNKNOWN>";
}

#endif

const char*
_rb_gles2_error_message(GLenum err) {
	switch (err) {
	case GL_NO_ERROR:
		return "GL_NO_ERROR";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	default:
		break;
	}
	return "<UNKNOWN>";
}

bool
_rb_gles2_check_error(const char* message, size_t length) {
	GLenum err = glGetError();
	if (err != GL_NONE) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("%.*s: %s", STRING_FORMAT(message),
		           _rb_gles2_error_message(err));
		           FOUNDATION_ASSERT_FAILFORMAT("OpenGLES2 error: %.*s: %s", STRING_FORMAT(message),
		                                        _rb_gles2_error_message(err));
		           return true;
	}
	return false;
}

static bool
_rb_gles2_log_error(const char* message, size_t length) {
	if (!_rb_gles2_check_error(message, length)) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("%.*s: No GL error"),
		           STRING_FORMAT(message));
		return false;
	}
	return true;
}

static bool
_rb_gles2_construct(render_backend_t* backend) {
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;

#if FOUNDATION_PLATFORM_IOS

	const void* context = _rb_gles2_ios_create_egl_context();
	if (!context)
		return false;

	backend_gles2->context = context;

#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	int major = 2;
	int minor = 0;
	if (!eglInitialize(display, &major, &minor)) {
		log_warn(HASH_RENDER, WARNING_SYSTEM_CALL_FAIL, STRING_CONST("Unable to initialize EGL");
		         return false;
	}
	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		log_warn(HASH_RENDER, WARNING_SYSTEM_CALL_FAIL, STRING_CONST("Unable to bind OpenGL ES API");
		         return false;
	}
	log_infof(HASH_RENDER, STRING_CONST("Initialized EGL v%d.%d"), major, minor);

	backend_gles2->display = display;

#else
	FOUNDATION_ASSERT_FAIL("GLES2 render backend platform not implemented");
	return error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
#endif

	backend_gles2->render_thread = thread_id();

	log_debug(HASH_RENDER, STRING_CONST("Constructed GLES2 render backend");
	          return true;
}

static void
_rb_gles2_destruct(render_backend_t* backend) {
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;

#if FOUNDATION_PLATFORM_IOS

	_rb_gles2_ios_destroy_egl_context(backend_gles2->context);

	backend_gles2->context = 0;

#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

	eglMakeCurrent(backend_gles2->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	eglTerminate(backend_gles2->display);

	backend_gles2->display = 0;

#  if FOUNDATION_PLATFORM_ANDROID
	if (backend_gles2->drawable->native)
		ANativeWindow_release((ANativeWindow*)backend_gles2->drawable->native);
#  elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	//TODO: Implement
#  endif

#else
	FOUNDATION_ASSERT_FAIL("GLES2 render backend platform not implemented");
	error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
	return;
#endif

	log_debug(HASH_RENDER, STRING_CONST("Destructed GLES2 render backend"));
}


static size_t
_rb_gles2_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	if (capacity)
		store[0] = (unsigned int)WINDOW_ADAPTER_DEFAULT;
	return 1;
}

static size_t
_rb_gles2_enumerate_modes(render_backend_t* backend, unsigned int adapter,
                         render_resolution_t* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(adapter);
	if (!capacity)
		return 1;

#if FOUNDATION_PLATFORM_IOS
	render_resolution_t mode = {
		0,
		_rb_gles2_ios_screen_width(),
		_rb_gles2_ios_screen_height(),
		PIXELFORMAT_R8G8B8X8,
		COLORSPACE_LINEAR,
		60
	};
#else
	render_resolution_t mode = { 0, 800, 600, PIXELFORMAT_R8G8B8X8, COLORSPACE_LINEAR, 60 };
#endif
	store[0] = mode;
	return 1;
}

static void
_rb_gles2_enable_thread(render_backend_t* backend) {
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;
#if FOUNDATION_PLATFORM_IOS
	_rb_gles2_ios_set_current_egl_context(backend_gles2->context);
#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI
	if (!eglMakeCurrent(backend_gles2->display, backend_gles2->surface, backend_gles2->surface,
	                    backend_gles2->context))
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to make context current on thread: %s"), eglGetErrorMessage(eglGetError()));
#else
	FOUNDATION_ASSERT_FAIL("GLES2 render backend platform not implemented");
	error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
#endif
}

static void
_rb_gles2_disable_thread(render_backend_t* backend) {
}

static bool
_rb_gles2_set_drawable(render_backend_t* backend, render_drawable_t* drawable) {
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;

	if (drawable->type == RENDERDRAWABLE_OFFSCREEN) {
		FOUNDATION_ASSERT_FAIL("GLES2 offscreen drawable not implemented");
		return error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
	}

#if FOUNDATION_PLATFORM_IOS

	glGenFramebuffers(1, &backend_gles2->framebuffer_id);
	glBindFramebuffer(GL_FRAMEBUFFER, backend_gles2->framebuffer_id);
	_rb_gles2_check_error("Unable to generate framebuffer object");

	glGenRenderbuffers(1, &backend_gles2->renderbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, backend_gles2->renderbuffer_id);
	_rb_gles2_check_error("Unable to generate renderbuffer object");

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
	                          backend_gles2->renderbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, backend_gles2->renderbuffer_id);
	_rb_gles2_ios_render_buffer_storage_from_drawable(backend_gles2->context, drawable->drawable,
	                                                  backend_gles2->framebuffer_id, backend_gles2->renderbuffer_id);

	GLint width = 0, height = 0;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);

	glGenRenderbuffers(1, &backend_gles2->depthbuffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, backend_gles2->depthbuffer_id);
	_rb_gles2_check_error("Unable to generate depthbuffer object");

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
	                          backend_gles2->depthbuffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
	_rb_gles2_check_error("Unable to allocate storage for depthbuffer object");

	glBindRenderbuffer(GL_RENDERBUFFER, backend_gles2->renderbuffer_id);   //restore color binding
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Failed to make complete framebuffer object with dimensions %dx%d: %d"), width, height,
		           glCheckFramebufferStatus(GL_FRAMEBUFFER));
		return false;
	}

	log_infof(HASH_RENDER,
	          STRING_CONST("Initialized complete framebuffer object with dimensions %dx%d"), width, height);

	backend_gles2->framebuffer_width = width;
	backend_gles2->framebuffer_height = height;

#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

	FOUNDATION_ASSERT_MSG(drawable->display == backend_gles2->display, "Invalid drawable display");
	eglMakeCurrent(drawable->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

#define MAX_CONFIG_ATTRS_SIZE 32
#define MAX_MATCHING_CONFIGS  64

	EGLint config_attrs[MAX_CONFIG_ATTRS_SIZE];
	EGLint i = 0;
	int ret = 0;

	// construct attribute request
	config_attrs[i]   = EGL_SURFACE_TYPE;
	config_attrs[++i] = EGL_WINDOW_BIT;
	config_attrs[++i] = EGL_RENDERABLE_TYPE;
	config_attrs[++i] = EGL_OPENGL_ES2_BIT;
	config_attrs[++i] = EGL_RED_SIZE;
	config_attrs[++i] = 8;
	config_attrs[++i] = EGL_GREEN_SIZE;
	config_attrs[++i] = 8;
	config_attrs[++i] = EGL_BLUE_SIZE;
	config_attrs[++i] = 8;
	config_attrs[++i] = EGL_ALPHA_SIZE;
	config_attrs[++i] = 0;
	config_attrs[++i] = EGL_DEPTH_SIZE;
	config_attrs[++i] = 0;//16;
	config_attrs[++i] = EGL_STENCIL_SIZE;
	config_attrs[++i] = 0;
	config_attrs[++i] = EGL_NONE;

	FOUNDATION_ASSERT(i < MAX_CONFIG_ATTRS_SIZE);

	// choose configs
	EGLConfig matching_configs[MAX_MATCHING_CONFIGS];
	EGLint num_matching_configs = 0;
	if (!eglChooseConfig(drawable->display, config_attrs, matching_configs, MAX_MATCHING_CONFIGS,
	                     &num_matching_configs)) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to find suitable EGL config: %s (display %" PRIfixPTR ")",
		                        eglGetErrorMessage(eglGetError()), drawable->display);
		           return false;
	}

	if (!num_matching_configs) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		          STRING_CONST("Unable to find suitable EGL config: no matching config"));
		return false;
	}

	backend_gles2->config = matching_configs[0];

#if FOUNDATION_PLATFORM_ANDROID

	ANativeWindow_acquire((ANativeWindow*)drawable->native);

	EGLint format = 0;
	eglGetConfigAttrib(drawable->display, backend_gles2->config, EGL_NATIVE_VISUAL_ID, &format);
	log_debugf(HASH_RENDER, STRING_CONST("Display config native visual ID: %d"), format);

	int drawable_width = render_drawable_width(drawable);
	int drawable_height = render_drawable_height(drawable);
	ret = ANativeWindow_setBuffersGeometry((ANativeWindow*)drawable->native, drawable_width,
	                                       drawable_height, format);
	log_debugf(HASH_RENDER, STRING_CONST("Window setBuffersGeometry( %d, %d ): %d"), drawable_width,
	           drawable_height, ret);

	backend_gles2->surface = eglCreateWindowSurface(drawable->display, backend_gles2->config,
	                                                (EGLNativeWindowType)drawable->native, 0);
	if (!backend_gles2->surface) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create EGL surface: %s"),
		           eglGetErrorMessage(eglGetError()));
		return false;
	}

#elif FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T  dispman_update;

	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	uint32_t screen_width = 0;
	uint32_t screen_height = 0;
	int success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
	if (success < 0) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to get screen dimensions: %d"),
		           success);
		return false;
	}

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = screen_width;
	dst_rect.height = screen_height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = screen_width << 16;
	src_rect.height = screen_height << 16;

	dispman_display = vc_dispmanx_display_open(0 /* LCD */);
	dispman_update = vc_dispmanx_update_start(0);

	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
	                                          0/*layer*/, &dst_rect, 0/*src*/,
	                                          &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

	if (!drawable->native)
		drawable->native = allocate(render_allocator(), sizeof(EGL_DISPMANX_WINDOW_T), 0);
	EGL_DISPMANX_WINDOW_T* native_window = drawable->native;
	native_window->element = dispman_element;
	native_window->width   = screen_width;
	native_window->height  = screen_height;

	vc_dispmanx_update_submit_sync(dispman_update);

	backend_gles2->surface = eglCreateWindowSurface(drawable->display, backend_gles2->config,
	                                                (EGLNativeWindowType)native_window, 0);
	if (!backend_gles2->surface) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create EGL surface: %s"),
		           eglGetErrorMessage(eglGetError()));
		return false;
	}

	drawable->width = screen_width;
	drawable->height = screen_height;

#endif

	EGLint context_attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	backend_gles2->context = eglCreateContext(drawable->display, backend_gles2->config, EGL_NO_CONTEXT,
	                                          context_attrs);
	if (!backend_gles2->context) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to create EGL context: %s"),
		           eglGetErrorMessage(eglGetError()));
		return false;
	}

	eglMakeCurrent(drawable->display, backend_gles2->surface, backend_gles2->surface,
	               backend_gles2->context);

	EGLint width = 0, height = 0;
	eglQuerySurface(drawable->display, backend_gles2->surface, EGL_WIDTH, &width);
	eglQuerySurface(drawable->display, backend_gles2->surface, EGL_HEIGHT, &height);

	backend_gles2->framebuffer_width = width;
	backend_gles2->framebuffer_height = height;

	log_debugf(HASH_RENDER, STRING_CONST("Window initialized for EGL rendering: 0x%" PRIfixPTR
	                                     " dimensions %dx%d"), drawable->native, backend_gles2->framebuffer_width,
	           backend_gles2->framebuffer_height);

	log_debugf(HASH_RENDER, STRING_CONST("Vendor     : %s"), eglQueryString(drawable->display,
	           EGL_VENDOR));
	log_debugf(HASH_RENDER, STRING_CONST("Version    : %s"), eglQueryString(drawable->display,
	           EGL_VERSION));
	log_debugf(HASH_RENDER, STRING_CONST("Extensions : %s"), eglQueryString(drawable->display,
	           EGL_EXTENSIONS));
	log_debugf(HASH_RENDER, STRING_CONST("Client APIs: %s"), eglQueryString(drawable->display,
	           EGL_CLIENT_APIS));

	//#if !FOUNDATION_BUILD_RTM
	int attributes[33] = {
		EGL_BUFFER_SIZE,
		EGL_ALPHA_SIZE,
		EGL_BLUE_SIZE,
		EGL_GREEN_SIZE,
		EGL_RED_SIZE,
		EGL_DEPTH_SIZE,
		EGL_STENCIL_SIZE,
		EGL_CONFIG_CAVEAT,
		EGL_CONFIG_ID,
		EGL_LEVEL,
		EGL_MAX_PBUFFER_HEIGHT,
		EGL_MAX_PBUFFER_PIXELS,
		EGL_MAX_PBUFFER_WIDTH,
		EGL_NATIVE_RENDERABLE,
		EGL_NATIVE_VISUAL_ID,
		EGL_NATIVE_VISUAL_TYPE,
		0x3030, //EGL_PRESERVED_RESOURCES,
		EGL_SAMPLES,
		EGL_SAMPLE_BUFFERS,
		EGL_SURFACE_TYPE,
		EGL_TRANSPARENT_TYPE,
		EGL_TRANSPARENT_RED_VALUE,
		EGL_TRANSPARENT_GREEN_VALUE,
		EGL_TRANSPARENT_BLUE_VALUE,
		EGL_BIND_TO_TEXTURE_RGB,
		EGL_BIND_TO_TEXTURE_RGBA,
		EGL_MIN_SWAP_INTERVAL,
		EGL_MAX_SWAP_INTERVAL,
		EGL_LUMINANCE_SIZE,
		EGL_ALPHA_MASK_SIZE,
		EGL_COLOR_BUFFER_TYPE,
		EGL_RENDERABLE_TYPE,
		EGL_CONFORMANT
	};
	const char* attribnames[33] = {
		"EGL_BUFFER_SIZE",
		"EGL_ALPHA_SIZE",
		"EGL_BLUE_SIZE",
		"EGL_GREEN_SIZE",
		"EGL_RED_SIZE",
		"EGL_DEPTH_SIZE",
		"EGL_STENCIL_SIZE",
		"EGL_CONFIG_CAVEAT",
		"EGL_CONFIG_ID",
		"EGL_LEVEL",
		"EGL_MAX_PBUFFER_HEIGHT",
		"EGL_MAX_PBUFFER_PIXELS",
		"EGL_MAX_PBUFFER_WIDTH",
		"EGL_NATIVE_RENDERABLE",
		"EGL_NATIVE_VISUAL_ID",
		"EGL_NATIVE_VISUAL_TYPE",
		"EGL_PRESERVED_RESOURCES",
		"EGL_SAMPLES",
		"EGL_SAMPLE_BUFFERS",
		"EGL_SURFACE_TYPE",
		"EGL_TRANSPARENT_TYPE",
		"EGL_TRANSPARENT_RED_VALUE",
		"EGL_TRANSPARENT_GREEN_VALUE",
		"EGL_TRANSPARENT_BLUE_VALUE",
		"EGL_BIND_TO_TEXTURE_RGB",
		"EGL_BIND_TO_TEXTURE_RGBA",
		"EGL_MIN_SWAP_INTERVAL",
		"EGL_MAX_SWAP_INTERVAL",
		"EGL_LUMINANCE_SIZE",
		"EGL_ALPHA_MASK_SIZE",
		"EGL_COLOR_BUFFER_TYPE",
		"EGL_RENDERABLE_TYPE",
		"EGL_CONFORMANT"
	};
	for (int ia = 0; ia < 33; ++ia) {
		EGLint value = 0;
		if (eglGetConfigAttrib(drawable->display, backend_gles2->config, attributes[ia], &value))
			log_debugf(HASH_RENDER, STRING_CONST("%s: %d"), attribnames[ia], value);
		else
			log_debugf(HASH_RENDER, STRING_CONST("%s: <failed>"), attribnames[ia]);
		eglGetError();
	}
	//#endif

#else
	FOUNDATION_ASSERT_FAIL("Platform not implemented");
	return error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
#endif

	glBlendFunc(GL_ONE, GL_ZERO);
	glEnable(GL_BLEND);

	glLineWidth(1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMaskSeparate(GL_FRONT_AND_BACK, 0xFFFFFFFF);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glViewport(0, 0, backend_gles2->framebuffer_width, backend_gles2->framebuffer_height);

	_rb_gles2_check_error("Error setting up default state");

	return true;
}

static void
_rb_gles2_dispatch(render_backend_t* backend, render_context_t** contexts, size_t num_contexts) {
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;

	for (size_t context_index = 0, context_size = num_contexts; context_index < context_size;
	        ++context_index) {
		render_context_t* context = contexts[context_index];
		render_command_t* command = context->commands;
		const radixsort_index_t* order = context->order;

		for (int cmd_index = 0, cmd_size = atomic_load32(&context->reserved); cmd_index < cmd_size;
		        ++cmd_index, ++order) {
			command = context->commands + *order;
			switch (command->type) {
			case RENDERCOMMAND_CLEAR: {
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
						glClearDepthf(command->data.clear.depth);
					}

					if (buffer_mask & RENDERBUFFER_STENCIL) {
						//glClearStencil( command->data.clear.stencil );
						bits |= GL_STENCIL_BUFFER_BIT;
					}

					if (backend_gles2->use_clear_scissor)
						glEnable(GL_SCISSOR_TEST);

					glClear(bits);

					if (backend_gles2->use_clear_scissor)
						glDisable(GL_SCISSOR_TEST);
					glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

					break;
				}

			case RENDERCOMMAND_VIEWPORT: {
					int target_width = render_target_width(context->target);
					int target_height = render_target_height(context->target);

					GLint x = command->data.viewport.x;
					GLint y = command->data.viewport.y;
					GLsizei w = command->data.viewport.width;
					GLsizei h = command->data.viewport.height;

					glViewport(x, y, w, h);
					glScissor(x, y, w, h);

					backend_gles2->use_clear_scissor = (x || y || (w != target_width) || (h != target_height));
					break;
				}

				/*	case RENDERCOMMAND_RENDER:
					{
						render_vertexbuffer_t* vertexbuffer = pool_lookup( _global_pool_renderbuffer, command->data.render.vertexbuffer );
						render_indexbuffer_t* indexbuffer  = pool_lookup( _global_pool_renderbuffer, command->data.render.indexbuffer );
						render_vertexshader_t* vertexshader = pool_lookup( _global_pool_shader, command->data.render.vertexshader );
						render_pixelshader_t* pixelshader  = pool_lookup( _global_pool_shader, command->data.render.pixelshader );
						render_parameter_block_t* block = pool_lookup( _global_pool_parameterblock, command->data.render.parameterblock );

						if( !vertexbuffer || !indexbuffer || !vertexshader || !pixelshader ) //Outdated references
						{
							NEO_ASSERT_FAIL( "Render command using invalid resources" );
							break;
						}

						if( vertexbuffer->flags & RENDERBUFFER_DIRTY )
							_rb_gles2_upload_buffer( backend, (render_buffer_t*)vertexbuffer );
						if( indexbuffer->flags & RENDERBUFFER_DIRTY )
							_rb_gles2_upload_buffer( backend, (render_buffer_t*)indexbuffer );

						//Bind vertex attributes
						{
							glBindBuffer( GL_ARRAY_BUFFER, (unsigned int)vertexbuffer->backend_data[0] );

							const render_vertex_decl_t* decl = &vertexbuffer->decl;
							for( unsigned int attrib = 0; attrib < VERTEXATTRIBUTE_NUMATTRIBUTES; ++attrib )
							{
								const uint8_t format = decl->attribute[attrib].format;
								if( format < VERTEXFORMAT_NUMTYPES )
								{
									glVertexAttribPointer( attrib, _rb_gles2_vertex_format_size[format], _rb_gles2_vertex_format_type[format], _rb_gles2_vertex_format_norm[format], vertexbuffer->size, (const void*)(uintptr_t)decl->attribute[attrib].offset );
									glEnableVertexAttribArray( attrib );
								}
								else
								{
									glDisableVertexAttribArray( attrib );
								}
							}
						}

						//Index buffer
						glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (unsigned int)indexbuffer->backend_data[0] );

						//Bind programs/shaders
						render_program_gles2_t* program = _rb_gles2_program_map( (unsigned int)vertexshader->backend_data[0], (unsigned int)pixelshader->backend_data[0] );
						glUseProgram( program->program );

						// Bind the parameter blocks
						render_parameter_info_t* param_info = block->info;
						hash_t* param_name = pointer_offset( block, sizeof( render_parameter_block_t ) + ( sizeof( render_parameter_info_t ) * block->num ) );
						for( unsigned int ip = 0; ip < block->num; ++ip, ++param_info, ++param_name )
						{
							if( param_info->type == RENDERPARAMETER_TEXTURE )
							{
								//TODO: Dynamic use of texture units, reusing unit that already have correct texture bound, and least-recently-used evicting old bindings to free a new unit
								glActiveTexture( GL_TEXTURE0 + param_info->unit );

								object_t object = *(object_t*)pointer_offset( block, param_info->offset );
								render_texture_gles2_t* texture = object ? pool_lookup( _global_pool_texture, object ) : 0;
								NEO_ASSERT_MSGFORMAT( !object || texture, "Parameter block using old/invalid texture 0x%llx", object );

								glBindTexture( GL_TEXTURE_2D, texture ? texture->object : 0 );

								for( unsigned int iu = 0; iu < program->num_uniforms; ++iu )
								{
									if( program->uniforms[iu].name == *param_name )
									{
										glUniform1i( program->uniforms[iu].location, param_info->unit );
										break;
									}
								}
							}
							else
							{
								for( unsigned int iu = 0; iu < program->num_uniforms; ++iu )
								{
									if( program->uniforms[iu].name == *param_name )
									{
										if( param_info->type == RENDERPARAMETER_FLOAT4 )
											glUniform4fv( program->uniforms[iu].location, param_info->dim, (const GLfloat*)pointer_offset( block, param_info->offset ) );
										else if( param_info->type == RENDERPARAMETER_INT4 )
											glUniform4iv( program->uniforms[iu].location, param_info->dim, (const GLint*)pointer_offset( block, param_info->offset ) );
										else if( param_info->type == RENDERPARAMETER_MATRIX )
											glUniformMatrix4fv( program->uniforms[iu].location, param_info->dim, GL_FALSE, (const GLfloat*)pointer_offset( block, param_info->offset ) );
										break;
									}
								}
							}
						}

						//TODO: Proper states
						//ID3D10Device_RSSetState( device, backend_dx10->rasterizer_state[0].state );

						//FLOAT blend_factors[] = { 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f };
						//ID3D10Device_OMSetBlendState( device, backend_dx10->blend_state[ ( command->data.render.blend_state >> 48ULL ) & 0xFFFFULL ].state, blend_factors, 0xFFFFFFFF );
						//ID3D10Device_OMSetDepthStencilState( device, backend_dx10->depthstencil_state[0].state, 0xFFFFFFFF );

						if( command->data.render.blend_state )
						{
							unsigned int source_color = (unsigned int)( command->data.render.blend_state & 0xFULL );
							unsigned int dest_color = (unsigned int)( ( command->data.render.blend_state >> 4ULL ) & 0xFULL );

							glBlendFunc( _rb_gles2_blend_func[source_color], _rb_gles2_blend_func[dest_color] );
						}
						else
						{
							glBlendFunc( GL_ONE, GL_ZERO );
						}

						unsigned int primitive = command->data.render.primitive;
						unsigned int num = command->data.render.num;
						unsigned int pnum = _rb_gles2_primitive_mult[primitive] * num + _rb_gles2_primitive_add[primitive];

						glDrawElements( _rb_gles2_primitive_type[primitive], pnum, ( indexbuffer->size == 2 ) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, 0 );

				#if !NEO_BUILD_RTM
						glesCheckError( "Failed rendering primitives" );
				#endif

						break;
					}*/
			}
		}
	}
}

static void
_rb_gles2_flip(render_backend_t* backend) {
	render_backend_gles2_t* backend_gles2 = (render_backend_gles2_t*)backend;

#if FOUNDATION_PLATFORM_IOS

	_rb_gles2_ios_present_render_buffer(backend_gles2->context);

#elif FOUNDATION_PLATFORM_ANDROID || FOUNDATION_PLATFORM_LINUX_RASPBERRYPI

	if (backend_gles2->surface)
		eglSwapBuffers(backend_gles2->display, backend_gles2->surface);

#else
#  error Not Implemented
#endif

	++backend->framecount;
}

static void*
_rb_gles2_allocate_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	return memory_allocate(HASH_RENDER, buffer->size * buffer->allocated, 0, MEMORY_PERSISTENT);
}

static void
_rb_gles2_deallocate_buffer(render_backend_t* backend, render_buffer_t* buffer, bool sys,
                            bool aux) {
	if (sys)
		memory_deallocate(buffer->store);

	if (aux && buffer->backend_data[0]) {
		GLuint buffer_object = (GLuint)buffer->backend_data[0];
		glDeleteBuffers(1, &buffer_object);
		buffer->backend_data[0] = 0;
	}
}

static bool
_rb_gles2_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	if ((buffer->buffertype == RENDERBUFFER_PARAMETER) || (buffer->buffertype == RENDERBUFFER_STATE))
		return true;

	GLuint buffer_object = (GLuint)buffer->backend_data[0];
	if (!buffer_object) {
		glGenBuffers(1, &buffer_object);
		if (_rb_gles2_check_error("Unable to create buffer object"))
			return false;
		buffer->backend_data[0] = buffer_object;
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer_object);
	glBufferData(GL_ARRAY_BUFFER, buffer->size * buffer->allocated, buffer->store,
	             (buffer->usage == RENDERUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	if (_rb_gles2_check_error("Unable to upload buffer object data"))
		return false;

	buffer->flags &= ~RENDERBUFFER_DIRTY;
	return true;
}

static bool
_rb_gles2_upload_shader(render_backend_t* backend, render_shader_t* shader, const void* buffer,
                        size_t size) {
	bool ret = false;
	switch (shader->shadertype) {
	case SHADER_VERTEX:
		//Vertex program backend data:
		//  0 - Shader object (GLuint)
		if (shader->backend_data[0]) {
			glDeleteShader((GLuint)shader->backend_data[0]);
			shader->backend_data[0] = 0;
		}

		GLuint shader_object = glCreateShader(GL_VERTEX_SHADER);
		if (!shader_object) {
			_rb_gles2_log_error("Unable to create vertex shader object");
			return;
		}
		shader->backend_data[0] = shader_object;

		if (size < 16) {
			log_errorf(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Invalid vertex shader code: %.*s"),
			           (int)size, (const char*)buffer);
			return;
		}
		//TODO: Binary formats
		GLint length = (GLint)size;
		glShaderSource(shader_object, 1, (const GLchar**)&buffer, &length);
		glCompileShader(shader_object);

		GLint compiled = 0;
		glGetShaderiv(shader_object, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint buffer_size = 2048;
			GLchar* log = memory_allocate(HASH_RENDER, buffer_size, 0, MEMORY_TEMPORARY);
			GLint log_length = 0;
			glGetShaderInfoLog(shader_object, buffer_size, &log_length, log);
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to compile vertex shader: %.*s"), (int)log_length, log);
			memory_deallocate(log);
			log_debugf(HASH_RENDER, STRING_CONST("Shader source:\n%.*s"), (int)size, (const char*)buffer);
			break;
		}

		if (_rb_gles2_check_error("Error uploading and compiling vertex shader"))
			break;

		ret = true;
		break;

	case SHADER_PIXEL:
		//Fragment program backend data:
		//  0 - Shader object (GLuint)
		if (shader->backend_data[0]) {
			glDeleteShader((GLuint)shader->backend_data[0]);
			shader->backend_data[0] = 0;
		}

		GLuint shader_object = glCreateShader(GL_FRAGMENT_SHADER);
		if (!shader_object) {
			_rb_gles2_log_error("Unable to create pixel shader object");
			return;
		}
		shader->backend_data[0] = shader_object;

		if (size < 16) {
			log_errorf(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Invalid pixel shader code: %.*s"),
			           (int)size, (const char*)buffer);
			return;
		}
		//TODO: Binary formats
		GLint length = (GLint)size;
		glShaderSource(shader_object, 1, (const GLchar**)&buffer, &length);
		glCompileShader(shader_object);

		GLint compiled = 0;
		glGetShaderiv(shader_object, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint buffer_size = 2048;
			GLchar* log = memory_allocate(HASH_RENDER, buffer_size, 0, MEMORY_TEMPORARY);
			GLint log_length = 0;
			glGetShaderInfoLog(shader_object, buffer_size, &log_length, log);
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to compile pixel shader: %.*s"), (int)log_length, log);
			memory_deallocate(log);
			log_debugf(HASH_RENDER, STRING_CONST("Shader source:\n%.*s"), (int)size, (const char*)buffer);
			break;
		}

		if (_rb_gles2_check_error("Error uploading and compiling pixel shader"))
			break;

		ret = true;
		break;

	default:
		break;
	}
	return ret;
}

static void
_rb_gles2_program_unmap(GLuint vertex_shader, GLuint pixel_shader) {
	/*for( int ip = 0; ip < 512; ++ip )
	{
		if( ( _rb_gles2_programs[ip].vertex_shader == vertex_shader ) || ( _rb_gles2_programs[ip].pixel_shader == pixel_shader ) )
		{
			glDeleteProgram( _rb_gles2_programs[ip].program );
			deallocate( _rb_gles2_programs[ip].uniforms );
			memset( _rb_gles2_programs + ip, 0, sizeof( render_program_gles2_t ) );
		}
	}*/
}

static void
_rb_gles2_link_buffer(render_backend_t* backend, render_buffer_t* buffer,
                      render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(program);
}

static void
_rb_gles2_deallocate_shader(render_backend_t* backend, render_shader_t* shader) {
	switch (shader->shadertype) {
	case SHADER_VERTEX: {
			//Vertex program backend data:
			//  0 - Shader object (GLuint)
			if (shader->backend_data[0]) {
				_rb_gles2_program_unmap((unsigned int)shader->backend_data[0], 0);
				glDeleteShader((GLuint)shader->backend_data[0]);
			}
			shader->backend_data[0] = 0;
			break;
		}

	case SHADER_PIXEL: {
			//Pixel shader backend data:
			//  0 - Shader object (GLuint)
			if (shader->backend_data[0]) {
				_rb_gles2_program_unmap(0, (unsigned int)shader->backend_data[0]);
				glDeleteShader((GLuint)shader->backend_data[0]);
			}
			shader->backend_data[0] = 0;
			break;
		}
	}
}

static void
_rb_gles2_deallocate_program(render_backend_t* backend, render_program_t* program) {
}

static render_backend_vtable_t _render_backend_vtable_gles2 = {
	.construct = _rb_gles2_construct,
	.destruct  = _rb_gles2_destruct,
	.enumerate_adapters = _rb_gles2_enumerate_adapters,
	.enumerate_modes = _rb_gles2_enumerate_modes,
	.enable_thread = _rb_gles2_enable_thread,
	.disable_thread = _rb_gles2_disable_thread,
	.set_drawable = _rb_gles2_set_drawable,
	.dispatch = _rb_gles2_dispatch,
	.flip = _rb_gles2_flip,
	.allocate_buffer = _rb_gles2_allocate_buffer,
	.link_buffer = _rb_gles2_link_buffer,
	.deallocate_buffer = _rb_gles2_deallocate_buffer,
	.upload_buffer = _rb_gles2_upload_buffer,
	.upload_shader = _rb_gles2_upload_shader,
	.deallocate_shader = _rb_gles2_deallocate_shader
};

render_backend_t*
render_backend_gles2_allocate(void) {
	render_backend_t* backend = memory_allocate(HASH_RENDER, sizeof(render_backend_gles2_t), 0,
	                                            MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_GLES2;
	backend->api_group = RENDERAPIGROUP_GLES;
	backend->vtable = _render_backend_vtable_gles2;
	return backend;
}

#else

render_backend_t*
render_backend_gles2_allocate(void) {
	return 0;
}

#endif
