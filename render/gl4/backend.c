/* backend.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson
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
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include <foundation/foundation.h>
#include <window/window.h>
#include <resource/hashstrings.h>
#include <render/render.h>
#include <render/internal.h>

#include <render/gl4/backend.h>

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic push
#if __has_warning("-Wdeprecated-declarations")
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

#if FOUNDATION_PLATFORM_WINDOWS || \
    (FOUNDATION_PLATFORM_LINUX && !FOUNDATION_PLATFORM_LINUX_RASPBERRYPI)

#include <render/gl4/glwrap.h>

#if RENDER_ENABLE_NVGLEXPERT
#include <nvapi.h>

static void
nvoglexpert_callback(unsigned int category, unsigned int id, unsigned int detail, int object, const char* msg) {
	log_warnf(HASH_RENDER, STRING_CONST("nVidia OpenGL Expert error: Category 0x%08x, Message 0x%08x : %s"), category,
	          id, msg);
}

#endif

#include <render/gl4/glprocs.h>

typedef struct render_backend_gl4_t {
	RENDER_DECLARE_BACKEND;

	void* context;
	atomic32_t context_used;

	void** concurrent_context;
	atomic32_t* concurrent_used;
	void* concurrent_buffer;

	render_resolution_t resolution;

	bool use_clear_scissor;
} render_backend_gl4_t;

const char*
rb_gl_error_message(GLenum err) {
	switch (err) {
		case GL_NONE:
			return "GL_NONE";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		default:
			break;
	}
	return "<UNKNOWN>";
}

FOUNDATION_DECLARE_THREAD_LOCAL(void*, gl_context, 0)

bool
rb_gl_check_error(const char* message) {
	GLenum err = glGetError();
	if (err != GL_NONE) {
		const char* glmessage = rb_gl_error_message(err);
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("%s: %s"), message, glmessage);
		FOUNDATION_ASSERT_FAILFORMAT("OpenGL error: %s: %s", message, glmessage);
		return true;
	}
	return false;
}

void*
rb_gl_get_thread_context(void) {
	return get_thread_gl_context();
}

void
rb_gl_set_thread_context(void* context) {
	set_thread_gl_context(context);
}

#if BUILD_DEBUG
#if !FOUNDATION_PLATFORM_MACOS
static void APIENTRY
rb_gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
                     const void* user_param) {
	FOUNDATION_UNUSED(source);
	FOUNDATION_UNUSED(type);
	FOUNDATION_UNUSED(id);
	FOUNDATION_UNUSED(severity);
	FOUNDATION_UNUSED(user_param);

	if (severity == GL_DEBUG_SEVERITY_HIGH)
		log_errorf(HASH_RENDER, ERROR_INTERNAL_FAILURE, STRING_CONST("OpenGL: %.*s"), (int)length, message);
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
		log_warnf(HASH_RENDER, WARNING_SUSPICIOUS, STRING_CONST("OpenGL: %.*s"), (int)length, message);
	else if (severity == GL_DEBUG_SEVERITY_LOW)
		log_infof(HASH_RENDER, STRING_CONST("OpenGL: %.*s"), (int)length, message);
	else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		log_debugf(HASH_RENDER, STRING_CONST("OpenGL: %.*s"), (int)length, message);
}
#endif
#endif

void
rb_gl_destroy_context(const render_drawable_t* drawable, void* context) {
	if (!context)
		return;
	glFinish();
#if FOUNDATION_PLATFORM_WINDOWS
	FOUNDATION_UNUSED(drawable);
	if (wglGetCurrentContext() == context)
		wglMakeCurrent(0, 0);
	wglDeleteContext((HGLRC)context);
#elif FOUNDATION_PLATFORM_LINUX
	glXDestroyContext(drawable->display, context);
#elif FOUNDATION_PLATFORM_MACOS
	FOUNDATION_UNUSED(drawable);
	rb_gl_destroy_agl_context(context);
#endif
}

#if FOUNDATION_PLATFORM_WINDOWS

static bool wgl_initialized;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

RENDER_EXTERN void
rb_gl_init_gl_extensions(void) {
	if (wgl_initialized)
		return;
	wgl_initialized = true;

	window_t window_dummy;
	window_create(&window_dummy, WINDOW_ADAPTER_DEFAULT, STRING_CONST("__render_gl_init"), 10, 10, WINDOW_FLAG_NOSHOW);
	render_drawable_t drawable;
	render_drawable_initialize_window(&drawable, &window_dummy, 0);

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	HDC hdc = (HDC)drawable.hdc;
	int pixel_format = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixel_format, &pfd);
	DescribePixelFormat(hdc, pixel_format, sizeof(pfd), &pfd);
	HGLRC hglrc = wglCreateContext(hdc);
	if (!hglrc) {
		log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
		         STRING_CONST("Unable to initialize WGL extensions, unable to create default GL context"));
	} else if (!wglMakeCurrent(hdc, hglrc)) {
		log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
		         STRING_CONST("Unable to initialize WGL extensions, unable to make default GL context current"));
	} else {
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)rb_gl_get_proc_address("wglChoosePixelFormatARB");
		wglCreateContextAttribsARB =
		    (PFNWGLCREATECONTEXTATTRIBSARBPROC)rb_gl_get_proc_address("wglCreateContextAttribsARB");
	}

	wglMakeCurrent(0, 0);
	if (hglrc)
		wglDeleteContext(hglrc);
	window_finalize(&window_dummy);
	render_drawable_finalize(&drawable);
}

#else

RENDER_EXTERN void
rb_gl_init_gl_extensions(void) {
}

#endif

void*
rb_gl_create_context(const render_drawable_t* drawable, unsigned int major, unsigned int minor,
                     const pixelformat_t pixelformat, const colorspace_t colorspace, void** concurrent_contexts,
                     size_t concurrent_count) {
	FOUNDATION_UNUSED(pixelformat);
#if FOUNDATION_PLATFORM_WINDOWS
	HDC hdc = (HDC)drawable->hdc;

	// First set pixel format
	if (wglChoosePixelFormatARB) {
		int attributes[] = {WGL_DRAW_TO_WINDOW_ARB,
		                    GL_TRUE,
		                    WGL_SUPPORT_OPENGL_ARB,
		                    GL_TRUE,
		                    WGL_DOUBLE_BUFFER_ARB,
		                    GL_TRUE,
		                    WGL_PIXEL_TYPE_ARB,
		                    WGL_TYPE_RGBA_ARB,
		                    WGL_COLOR_BITS_ARB,
		                    (pixelformat == PIXELFORMAT_R8G8B8A8) ? 32 : 24,
		                    WGL_ALPHA_BITS_ARB,
		                    (pixelformat == PIXELFORMAT_R8G8B8A8) ? 8 : 0,
		                    WGL_DEPTH_BITS_ARB,
		                    32,
		                    WGL_ACCELERATION_ARB,
		                    WGL_FULL_ACCELERATION_ARB,
		                    0};
		size_t attributes_count = sizeof(attributes) / sizeof(attributes[0]);

		int pixel_format = 0;
		UINT formats_count = 0;
		wglChoosePixelFormatARB(hdc, attributes, 0, 1, &pixel_format, &formats_count);
		if (!formats_count) {
			// Try reducing depth bits to 24
			attributes[attributes_count - 4] = 24;
			wglChoosePixelFormatARB(hdc, attributes, 0, 1, &pixel_format, &formats_count);
		}
		if (!formats_count) {
			// Try skipping acceleration specifier
			attributes[attributes_count - 3] = 0;
			wglChoosePixelFormatARB(hdc, attributes, 0, 1, &pixel_format, &formats_count);
		}
		if (!formats_count) {
			log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
			         STRING_CONST("Unable to create context, unable to choose pixel format"));
			return nullptr;
		}

		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		DescribePixelFormat(hdc, pixel_format, sizeof(pfd), &pfd);
		if (!SetPixelFormat(hdc, pixel_format, &pfd)) {
			log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
			         STRING_CONST("Unable to create context, unable to set pixel format"));
			return nullptr;
		}
	} else {
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		if (pixelformat == PIXELFORMAT_R8G8B8A8) {
			pfd.cColorBits = 32;
			pfd.cAlphaBits = 8;
		} else {
			pfd.cColorBits = 24;
		}
		pfd.cDepthBits = 24;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pixel_format = ChoosePixelFormat(hdc, &pfd);
		if (!pixel_format) {
			log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
			         STRING_CONST("Unable to create context, unable to choose pixel format"));
			return nullptr;
		}

		if (!SetPixelFormat(hdc, pixel_format, &pfd)) {
			log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
			         STRING_CONST("Unable to create context, unable to set pixel format"));
			return nullptr;
		}
	}

	// Now create context
	HGLRC hglrc = 0;
	if (wglCreateContextAttribsARB) {
		int context_flags = 0;
#if BUILD_DEBUG
		context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif
		int attributes[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
		                    major,
		                    WGL_CONTEXT_MINOR_VERSION_ARB,
		                    minor,
		                    WGL_CONTEXT_FLAGS_ARB,
		                    context_flags,
		                    WGL_CONTEXT_PROFILE_MASK_ARB,
		                    WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		                    0,
		                    0,
		                    0};
		size_t attributes_count = sizeof(attributes) / sizeof(attributes[0]);
		if ((colorspace == COLORSPACE_sRGB) && (major < 4)) {
			attributes[attributes_count - 3] = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
			attributes[attributes_count - 2] = GL_TRUE;
		}

		hglrc = wglCreateContextAttribsARB(hdc, 0, attributes);
		if (hglrc && wglMakeCurrent(hdc, hglrc)) {
			for (size_t icontext = 0; icontext < concurrent_count; ++icontext) {
				concurrent_contexts[icontext] = wglCreateContextAttribsARB(hdc, hglrc, attributes);
				if (!concurrent_contexts[icontext]) {
					int err = GetLastError();
					string_const_t errmsg = system_error_message(err);
					log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
					           STRING_CONST("Unable to create GL context for concurrency: %.*s (%08x)"),
					           STRING_FORMAT(errmsg), err);
					break;
				}
			}
		}
	} else {
		hglrc = wglCreateContext(hdc);
		if (hglrc && wglMakeCurrent(hdc, hglrc)) {
			for (size_t icontext = 0; icontext < concurrent_count; ++icontext) {
				concurrent_contexts[icontext] = wglCreateContext(hdc);
				if (!concurrent_contexts[icontext]) {
					int err = GetLastError();
					string_const_t errmsg = system_error_message(err);
					log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
					           STRING_CONST("Unable to create GL context for concurrency: %.*s (%08x)"),
					           STRING_FORMAT(errmsg), err);
					break;
				}
				if (!wglShareLists(concurrent_contexts[icontext], hglrc)) {
					int err = GetLastError();
					string_const_t errmsg = system_error_message(err);
					log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
					           STRING_CONST("Unable to share GL context for concurrency: %.*s (%08x)"),
					           STRING_FORMAT(errmsg), err);
					break;
				}
			}
		}
	}

	if (!hglrc) {
		int err = GetLastError();
		string_const_t errmsg = system_error_message(err);
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to create GL context for version %d.%d: %.*s (%08x)"), major, minor,
		           STRING_FORMAT(errmsg), err);
		return nullptr;
	}

	const char* version = (const char*)glGetString(GL_VERSION);
	unsigned int have_major = 0, have_minor = 0, have_revision = 0;
	string_const_t version_arr[3];
	size_t arrsize = string_explode(version, string_length(version), STRING_CONST("."), version_arr, 3, false);

	have_major = (arrsize > 0) ? string_to_uint(STRING_ARGS(version_arr[0]), false) : 0;
	have_minor = (arrsize > 1) ? string_to_uint(STRING_ARGS(version_arr[1]), false) : 0;
	have_revision = (arrsize > 2) ? string_to_uint(STRING_ARGS(version_arr[2]), false) : 0;

	bool supported = (have_major > major);
	if (!supported && ((have_major == major) && (have_minor >= minor)))
		supported = true;

	if (!supported) {
		log_warnf(HASH_RENDER, WARNING_UNSUPPORTED, STRING_CONST("GL version %d.%d not supported, got %d.%d (%s)"),
		          major, minor, have_major, have_minor, version);
		wglMakeCurrent(0, 0);
		wglDeleteContext(hglrc);
		hglrc = 0;
	}

	if (hglrc && (major == 2)) {
		// We require GL_ARB_framebuffer_object extension
		if (!rb_gl_check_extension(STRING_CONST("GL_ARB_framebuffer_object"))) {
			log_infof(HASH_RENDER, STRING_CONST("GL version %d.%d not supported, missing framebuffer extension"), major,
			          minor);
			wglMakeCurrent(0, 0);
			wglDeleteContext(hglrc);
			hglrc = 0;
		}
	}

	if (hglrc && (colorspace == COLORSPACE_sRGB)) {
		glEnable(GL_FRAMEBUFFER_SRGB);
		if (!rb_gl_check_error("Unable to enable sRGB framebuffer"))
			log_info(HASH_RENDER, STRING_CONST("sRGB framebuffer"));
	}

	if (!hglrc)
		wglMakeCurrent(0, 0);

	void* context = hglrc;

#elif FOUNDATION_PLATFORM_LINUX

	GLXContext context = 0;
	GLXFBConfig* fbconfig;
	Display* display = 0;
	int screen = 0;
	int config_count = 0;
	bool verify_only = false;

	if (!drawable) {
		display = XOpenDisplay(0);
		screen = DefaultScreen(display);
		verify_only = true;
	} else {
		display = drawable->display;
		screen = drawable->screen;
		if (!display) {
			log_warn(HASH_RENDER, WARNING_INVALID_VALUE, STRING_CONST("Invalid drawable"));
			goto failed;
		}
	}

	XLockDisplay(display);

	if (glXQueryExtension(display, 0, 0) != True) {
		log_warn(HASH_RENDER, WARNING_UNSUPPORTED, STRING_CONST("Unable to query GLX extension"));
		goto failed;
	}

	int major_glx = 0, minor_glx = 0;
	glXQueryVersion(display, &major_glx, &minor_glx);
	if (!verify_only)
		log_debugf(HASH_RENDER, STRING_CONST("GLX version %d.%d"), major_glx, minor_glx);

	fbconfig = glXGetFBConfigs(display, screen, &config_count);
	if (!verify_only)
		log_debugf(HASH_RENDER, STRING_CONST("Got %d configs"), config_count);
	if (fbconfig && (config_count > 0)) {
		PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribs =
		    (PFNGLXCREATECONTEXTATTRIBSARBPROC)rb_gl_get_proc_address("glXCreateContextAttribsARB");
		if (glXCreateContextAttribs) {
			int context_flags = 0;
#if BUILD_DEBUG
			context_flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
#endif
			int attributes[] = {GLX_CONTEXT_MAJOR_VERSION_ARB,
			                    (int)major,
			                    GLX_CONTEXT_MINOR_VERSION_ARB,
			                    (int)minor,
			                    GLX_CONTEXT_FLAGS_ARB,
			                    context_flags,
			                    GLX_CONTEXT_PROFILE_MASK_ARB,
			                    GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
			                    0,
			                    0,
			                    0};
			size_t attributes_count = sizeof(attributes) / sizeof(attributes[0]);
			if ((colorspace == COLORSPACE_sRGB) && (major < 4)) {
				attributes[attributes_count - 3] = GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB;
				attributes[attributes_count - 2] = GL_TRUE;
			}

			int ic = 0;
			for (ic = 0; ic < config_count; ++ic) {
				context = glXCreateContextAttribs(display, fbconfig[ic], nullptr, 1, attributes);
				if (context)
					break;
			}

			if (context) {
				for (size_t icontext = 0; icontext < concurrent_count; ++icontext) {
					concurrent_contexts[icontext] =
					    glXCreateContextAttribs(display, fbconfig[ic], context, 1, attributes);
					if (!concurrent_contexts[icontext]) {
						log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
						           STRING_CONST("Unable to create GL context for concurrency"));
						break;
					}
				}
			}
		} else {
			log_warn(HASH_RENDER, WARNING_UNSUPPORTED,
			         STRING_CONST("Unable to get glXCreateContextAttribs proc address"));
		}
	}

	if (context && (major == 2)) {
		// We require GL_ARB_framebuffer_object extension
		if (!rb_gl_check_extension(STRING_CONST("GL_ARB_framebuffer_object"))) {
			log_infof(HASH_RENDER, STRING_CONST("GL version %d.%d not supported, missing framebuffer extension"), major,
			          minor);
			glXDestroyContext(display, context);
			context = nullptr;
		}
	}

	if (context && (colorspace == COLORSPACE_sRGB)) {
		glEnable(GL_FRAMEBUFFER_SRGB);
		if (!rb_gl_check_error("Unable to enable sRGB framebuffer"))
			log_info(HASH_RENDER, STRING_CONST("sRGB framebuffer"));
	}

failed:

	XUnlockDisplay(display);

	if (!drawable) {
		if (context)
			glXDestroyContext(display, context);
		XCloseDisplay(display);
	}

#elif FOUNDATION_PLATFORM_MACOS

	bool supported = false;

	CGDirectDisplayID display = CGMainDisplayID();
	void* view = (drawable ? drawable->view : 0);
	if (drawable) {
		// TODO: Get display mask from view
		// display = CGOpenGLDisplayMaskToDisplayID( _adapter._id );
	}
	unsigned int displaymask = CGDisplayIDToOpenGLDisplayMask(display);
	void* context = rb_gl_create_agl_context(view, displaymask, 32 /*color_depth*/, 24 /*_res._depth*/,
	                                         8 /*_res._stencil*/, nullptr);
	if (!context) {
		log_warn(HASH_RENDER, WARNING_UNSUPPORTED, STRING_CONST("Unable to create OpenGL context"));
		goto failed;
	}

	const char* version = (const char*)glGetString(GL_VERSION);
	unsigned int have_major = 0, have_minor = 0, have_revision = 0;
	string_const_t version_arr[4];
	size_t tokens_count = string_explode(version, string_length(version), STRING_CONST("."), version_arr,
	                                     sizeof(version_arr) / sizeof(version_arr[0]), false);

	have_major = (tokens_count > 0) ? string_to_uint(STRING_ARGS(version_arr[0]), false) : 0;
	have_minor = (tokens_count > 1) ? string_to_uint(STRING_ARGS(version_arr[1]), false) : 0;
	have_revision = (tokens_count > 2) ? string_to_uint(STRING_ARGS(version_arr[2]), false) : 0;

	supported = (have_major > major);
	if (!supported && ((have_major == major) && (have_minor >= minor)))
		supported = true;

	if (!supported) {
		log_infof(HASH_RENDER, STRING_CONST("GL version %d.%d not supported, got %d.%d (%s)"), major, minor, have_major,
		          have_minor, version);
		goto failed;
	} else if (major == 2) {
		// We require GL_ARB_framebuffer_object extension
		if (!rb_gl_check_extension(STRING_CONST("GL_ARB_framebuffer_object"))) {
			log_infof(HASH_RENDER, STRING_CONST("GL version %d.%d not supported, missing framebuffer extension"), major,
			          minor);
			supported = false;
		}
	}

failed:

	if ((!supported || !drawable) && context)
		rb_gl_destroy_agl_context(context);

	if (!supported)
		context = nullptr;

	if (context && (colorspace == COLORSPACE_sRGB)) {
		glEnable(GL_FRAMEBUFFER_SRGB);
		if (!rb_gl_check_error("Unable to enable sRGB framebuffer"))
			log_info(HASH_RENDER, STRING_CONST("sRGB framebuffer"));
	}

	if (context) {
		for (size_t icontext = 0; icontext < concurrent_count; ++icontext) {
			concurrent_contexts[icontext] = rb_gl_create_agl_context(view, displaymask, 32 /*color_depth*/,
			                                                         24 /*_res._depth*/, 8 /*_res._stencil*/, context);
			if (!concurrent_contexts[icontext]) {
				log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
				           STRING_CONST("Unable to create GL context for concurrency"));
				break;
			}
		}
	}

#else
#error Not implemented
#endif

#if BUILD_DEBUG
#if !FOUNDATION_PLATFORM_MACOS
	PFNGLDEBUGMESSAGECONTROLPROC fnglDebugMessageControl =
	    (PFNGLDEBUGMESSAGECONTROLPROC)rb_gl_get_proc_address("glDebugMessageControl");
	if (fnglDebugMessageControl) {
		GLuint unused = 0;
		fnglDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unused, true);
	}
	PFNGLDEBUGMESSAGECALLBACKPROC fnglDebugMessageCallback =
	    (PFNGLDEBUGMESSAGECALLBACKPROC)rb_gl_get_proc_address("glDebugMessageCallback");
	if (fnglDebugMessageCallback) {
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		fnglDebugMessageCallback(rb_gl_debug_callback, nullptr);
	}
#endif
#endif

	return context;
}

bool
rb_gl_check_context(unsigned int major, unsigned int minor) {
	void* context = 0;

#if FOUNDATION_PLATFORM_WINDOWS

	window_t window_check;
	window_create(&window_check, WINDOW_ADAPTER_DEFAULT, STRING_CONST("__render_gl_check"), 10, 10, WINDOW_FLAG_NOSHOW);
	render_drawable_t drawable;
	render_drawable_initialize_window(&drawable, &window_check, 0);
	context = rb_gl_create_context(&drawable, major, minor, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR, 0, 0);
	wglMakeCurrent(0, 0);
	if (context)
		wglDeleteContext((HGLRC)context);
	window_finalize(&window_check);
	render_drawable_finalize(&drawable);

#elif FOUNDATION_PLATFORM_LINUX || FOUNDATION_PLATFORM_MACOS

	context = rb_gl_create_context(0, major, minor, PIXELFORMAT_R8G8B8, COLORSPACE_LINEAR, 0, 0);

#else
#error Not implemented
#endif

	return (context != 0);
}

bool
rb_gl_check_extension(const char* name, size_t length) {
	const char* ext = (const char*)glGetString(GL_EXTENSIONS);
	size_t extlength = string_length(ext);
	return string_find_string(ext, extlength, name, length, 0) != STRING_NPOS;
}

static void
rb_gl4_disable_thread(render_backend_t* backend) {
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

#if FOUNDATION_PLATFORM_LINUX
	XLockDisplay(backend->drawable.display);
#endif

	void* thread_context = rb_gl_get_thread_context();
	if (thread_context) {
		if (thread_context == backend_gl4->context) {
			atomic_store32(&backend_gl4->context_used, 0, memory_order_release);
#if FOUNDATION_PLATFORM_WINDOWS
			if (wglGetCurrentContext() == thread_context)
				wglMakeCurrent(0, 0);
#elif FOUNDATION_PLATFORM_LINUX
			if (glXGetCurrentContext() == thread_context)
				glXMakeCurrent(backend->drawable.display, None, nullptr);
#elif FOUNDATION_PLATFORM_MACOS
			if (rb_gl_agl_context_current() == thread_context)
				rb_gl_agl_make_context_current(nullptr);
#else
#error Not implemented
#endif
		} else {
			for (uint64_t icontext = 0; icontext < backend->concurrency; ++icontext) {
				if (backend_gl4->concurrent_context[icontext] == thread_context) {
#if FOUNDATION_PLATFORM_WINDOWS
					if (wglGetCurrentContext() == thread_context)
						wglMakeCurrent(0, 0);
#elif FOUNDATION_PLATFORM_LINUX
					if (glXGetCurrentContext() == thread_context)
						glXMakeCurrent(backend->drawable.display, None, nullptr);
#elif FOUNDATION_PLATFORM_MACOS
					if (rb_gl_agl_context_current() == thread_context)
						rb_gl_agl_make_context_current(nullptr);
#else
#error Not implemented
#endif
					atomic_store32(&backend_gl4->concurrent_used[icontext], 0, memory_order_release);
					break;
				}
			}
		}
	}

#if FOUNDATION_PLATFORM_LINUX
	XUnlockDisplay(backend->drawable.display);
#endif

	rb_gl_set_thread_context(0);
}

static void
rb_gl4_enable_thread(render_backend_t* backend) {
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	void* thread_context = rb_gl_get_thread_context();
	if (!thread_context) {
		if (!backend_gl4->context)
			return;
		if (atomic_cas32(&backend_gl4->context_used, 1, 0, memory_order_release, memory_order_acquire)) {
			thread_context = backend_gl4->context;
		} else {
			for (uint64_t icontext = 0; icontext < backend->concurrency; ++icontext) {
				if (atomic_cas32(&backend_gl4->concurrent_used[icontext], 1, 0, memory_order_release,
				                 memory_order_acquire)) {
					thread_context = backend_gl4->concurrent_context[icontext];
					break;
				}
			}
			if (!thread_context) {
				log_warn(HASH_RENDER, WARNING_INVALID_VALUE,
				         STRING_CONST("Unable to enable thread for GL4 rendering, no free concurrent context"));
				return;
			}
		}
		rb_gl_set_thread_context(thread_context);
	}

#if FOUNDATION_PLATFORM_WINDOWS
	if (!wglMakeCurrent((HDC)backend->drawable.hdc, (HGLRC)thread_context)) {
		string_const_t errmsg = system_error_message(0);
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to enable thread for GL4 rendering: %.*s"),
		           STRING_FORMAT(errmsg));
	}
#elif FOUNDATION_PLATFORM_LINUX
	XLockDisplay(backend->drawable.display);
	glXMakeCurrent(backend->drawable.display, (GLXDrawable)backend->drawable.drawable, thread_context);
	XUnlockDisplay(backend->drawable.display);
	rb_gl_check_error("Unable to enable thread for GL4 rendering");
#else
	FOUNDATION_ASSERT_FAIL("Platform not implemented");
	error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);
#endif
}

static bool
rb_gl4_construct(render_backend_t* backend) {
	FOUNDATION_UNUSED(backend);

	// TODO: Caps check
	// if( !... )
	//  return false;

	log_debug(HASH_RENDER, STRING_CONST("Constructed GL4 render backend"));
	return true;
}

static void
rb_gl4_destruct(render_backend_t* backend) {
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	for (uint64_t icontext = 0; icontext < backend->concurrency; ++icontext)
		rb_gl_destroy_context(&backend_gl4->drawable, backend_gl4->concurrent_context[icontext]);
	memory_deallocate(backend_gl4->concurrent_context);
	memory_deallocate(backend_gl4->concurrent_buffer);

	rb_gl4_disable_thread(backend);
	if (backend_gl4->context)
		rb_gl_destroy_context(&backend_gl4->drawable, backend_gl4->context);
	backend_gl4->context = 0;
	atomic_store32(&backend_gl4->context_used, 0, memory_order_release);

	log_debug(HASH_RENDER, STRING_CONST("Destructed GL4 render backend"));
}

size_t
rb_gl_enumerate_adapters(render_backend_t* backend, unsigned int* store, size_t capacity) {
	FOUNDATION_UNUSED(backend);
	if (capacity)
		store[0] = (unsigned int)WINDOW_ADAPTER_DEFAULT;
	return 1;
}

static bool
rb_gl4_set_drawable(render_backend_t* backend, const render_drawable_t* drawable) {
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;
	if (!FOUNDATION_VALIDATE_MSG(!backend_gl4->context, "Drawable switching not supported yet"))
		return error_report(ERRORLEVEL_ERROR, ERROR_NOT_IMPLEMENTED);

	if (backend_gl4->concurrency) {
		backend_gl4->concurrent_context = memory_allocate(HASH_RENDER, sizeof(void*) * backend_gl4->concurrency, 0,
		                                                  MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		backend_gl4->concurrent_buffer = memory_allocate(HASH_RENDER, sizeof(atomic32_t) * backend_gl4->concurrency, 0,
		                                                 MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
		backend_gl4->concurrent_used = backend_gl4->concurrent_buffer;
	}

	atomic_store32(&backend_gl4->context_used, 1, memory_order_release);
	backend_gl4->context = rb_gl_create_context(drawable, 4, 0, backend->pixelformat, backend->colorspace,
	                                            backend_gl4->concurrent_context, backend_gl4->concurrency);
	if (!backend_gl4->context) {
		log_error(HASH_RENDER, ERROR_UNSUPPORTED, STRING_CONST("Unable to create OpenGL 4 context"));
		atomic_store32(&backend_gl4->context_used, 0, memory_order_release);
		return false;
	}

	rb_gl_set_thread_context(backend_gl4->context);

	for (uint64_t icontext = 0; icontext < backend->concurrency; ++icontext) {
		if (!backend_gl4->concurrent_context[icontext]) {
			backend_gl4->concurrency = icontext;
			break;
		}
	}

#if FOUNDATION_PLATFORM_LINUX

	XLockDisplay(drawable->display);

	glXMakeCurrent(drawable->display, (GLXDrawable)drawable->drawable, backend_gl4->context);

	if (True == glXIsDirect(drawable->display, backend_gl4->context))
		log_debug(HASH_RENDER, STRING_CONST("Direct rendering enabled"));
	else
		log_warn(HASH_RENDER, WARNING_PERFORMANCE, STRING_CONST("Indirect rendering"));

	XUnlockDisplay(drawable->display);

#endif

#if RENDER_ENABLE_NVGLEXPERT
	NvAPI_OGL_ExpertModeSet(20, NVAPI_OGLEXPERT_REPORT_ALL, NVAPI_OGLEXPERT_OUTPUT_TO_CALLBACK, nvoglexpert_callback);
#endif

#if BUILD_ENABLE_LOG
	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	log_infof(HASH_RENDER, STRING_CONST("Vendor:     %s"), vendor ? vendor : "<unknown>");
	log_infof(HASH_RENDER, STRING_CONST("Renderer:   %s"), renderer ? renderer : "<unknown>");
	log_infof(HASH_RENDER, STRING_CONST("Version:    %s"), version ? version : "<unknown>");
#endif
	glGetError();

#if FOUNDATION_PLATFORM_WINDOWS
	const char* wglext = 0;
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = 0;
	if ((wglGetExtensionsStringARB =
	         (PFNWGLGETEXTENSIONSSTRINGARBPROC)rb_gl_get_proc_address("wglGetExtensionsStringARB")) != 0)
		wglext = wglGetExtensionsStringARB((HDC)drawable->hdc);
	else if ((wglGetExtensionsStringEXT =
	              (PFNWGLGETEXTENSIONSSTRINGEXTPROC)rb_gl_get_proc_address("wglGetExtensionsStringEXT")) != 0)
		wglext = wglGetExtensionsStringEXT();
	log_debugf(HASH_RENDER, STRING_CONST("WGL Extensions: %s"), wglext ? wglext : "<none>");
#endif

	if (!rb_gl_get_standard_procs(4, 0))
		return false;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);

	glViewport(0, 0, (GLsizei)drawable->width, (GLsizei)drawable->height);

	rb_gl_check_error("Error setting up default state");

	log_debug(HASH_RENDER, STRING_CONST("GL4 backend drawable initialized"));

	return true;
}

size_t
rb_gl_enumerate_modes(render_backend_t* backend, unsigned int adapter, render_resolution_t* store, size_t capacity) {
	size_t mode_count = 0;
	FOUNDATION_UNUSED(backend);

#if FOUNDATION_PLATFORM_LINUX
	FOUNDATION_ASSERT_MSG(adapter == WINDOW_ADAPTER_DEFAULT,
	                      "render_enumerate_modes not implemented when adapter is specified");

	Display* display = XOpenDisplay(0);
	if (!display) {
		log_error(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		          STRING_CONST("Unable to enumerate modes, unable to open display"));
		goto exit;
	}

	XLockDisplay(display);

	size_t store_count = 0;
	int depths[] = {15, 16, 24, 32};
	for (int i = 0; i < 4; ++i) {
		int visual_count = 0;
		XVisualInfo info;
		memset(&info, 0, sizeof(XVisualInfo));
		info.depth = depths[i];

		XVisualInfo* visual = XGetVisualInfo(display, VisualDepthMask, &info, &visual_count);
		for (int v = 0; v < visual_count; ++v) {
			XF86VidModeModeInfo** xmodes = 0;
			int xmodes_count = 0;

			int depth = 0, stencil = 0, color = 0;
			glXGetConfig(display, &visual[v], GLX_BUFFER_SIZE, &color);
			glXGetConfig(display, &visual[v], GLX_DEPTH_SIZE, &depth);
			glXGetConfig(display, &visual[v], GLX_STENCIL_SIZE, &stencil);

			if ((color < 24) || (depth < 15))
				continue;

			XF86VidModeGetAllModeLines(display, visual[v].screen, &xmodes_count, &xmodes);
			/*if( modes_count )
			qsort( xmodes, modes_count, sizeof( XF86VidModeModeInfo* ), Adapter::compareModes );*/

			for (int m = 0; m < xmodes_count; ++m) {
				if ((xmodes[m]->hdisplay < 600) || (xmodes[m]->vdisplay < 400))
					continue;

				int refresh = ((int)(0.5f + (1000.0f * (float)xmodes[m]->dotclock) /
				                                (float)(xmodes[m]->htotal * xmodes[m]->vtotal)));

				// 255 = MODE_BAD according to XFree sources...
				if (XF86VidModeValidateModeLine(display, visual[v].screen, xmodes[m]) == 255)
					continue;

				pixelformat_t format = PIXELFORMAT_R8G8B8A8;
				if (color == 24)
					format = PIXELFORMAT_R8G8B8;

				render_resolution_t mode = {0,      xmodes[m]->hdisplay, xmodes[m]->vdisplay,
				                            format, COLORSPACE_LINEAR,   (unsigned int)refresh};

				bool found = false;
				for (size_t c = 0; c < store_count; ++c) {
					if (!memcmp(store + c, &mode, sizeof(render_resolution_t))) {
						found = true;
						break;
					}
				}
				if (!found && (store_count < capacity))
					store[store_count++] = mode;
				++mode_count;
			}

			XFree(xmodes);
		}

		XFree(visual);
	}

	// Sort and index modes
	for (size_t c = 0, size = (mode_count < capacity ? mode_count : capacity); c < size; ++c)
		store[c].id = (unsigned int)c;

	if (!mode_count) {
		log_warnf(HASH_RENDER, WARNING_SUSPICIOUS,
		          STRING_CONST("Unable to enumerate resolutions for adapter %d, adding default fallback"), adapter);
		render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, COLORSPACE_LINEAR, 60};
		if (capacity)
			store[mode_count++] = mode;
	}

exit:

	XUnlockDisplay(display);
	XCloseDisplay(display);

#else
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(adapter);
	render_resolution_t mode = {0, 800, 600, PIXELFORMAT_R8G8B8A8, COLORSPACE_LINEAR, 60};
	if (capacity)
		store[mode_count++] = mode;
#endif

	return mode_count;
}

static void*
rb_gl4_allocate_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	return memory_allocate(HASH_RENDER, buffer->buffersize, 16, MEMORY_PERSISTENT);
}

static void
rb_gl4_deallocate_buffer(render_backend_t* backend, render_buffer_t* buffer, bool sys, bool aux) {
	FOUNDATION_UNUSED(backend);
	if (sys)
		memory_deallocate(buffer->store);

	if (aux) {
		if (buffer->backend_data[0]) {
			GLuint buffer_object = (GLuint)buffer->backend_data[0];
			glDeleteBuffers(1, &buffer_object);
			buffer->backend_data[0] = 0;
		}
		if (buffer->backend_data[1]) {
			GLuint vertex_array = (GLuint)buffer->backend_data[1];
			glDeleteVertexArrays(1, &vertex_array);
			buffer->backend_data[1] = 0;
		}
	}
}

static const GLint rb_gl4_vertex_format_size[VERTEXFORMAT_COUNT] = {1, 2, 3, 4, 4, 4, 1, 2, 4, 1, 2, 4};
static const GLenum rb_gl4_vertex_format_type[VERTEXFORMAT_COUNT] = {GL_FLOAT,         GL_FLOAT, GL_FLOAT, GL_FLOAT,
                                                                     GL_UNSIGNED_BYTE, GL_BYTE,  GL_SHORT, GL_SHORT,
                                                                     GL_SHORT,         GL_INT,   GL_INT,   GL_INT};
static const GLboolean rb_gl4_vertex_format_norm[VERTEXFORMAT_COUNT] = {GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE,
                                                                        GL_TRUE,  GL_TRUE,  GL_FALSE, GL_FALSE,
                                                                        GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE};

static bool
rb_gl4_upload_buffer(render_backend_t* backend, render_buffer_t* buffer) {
	FOUNDATION_UNUSED(backend);
	if ((buffer->buffertype == RENDERBUFFER_PARAMETER) || (buffer->buffertype == RENDERBUFFER_STATE))
		return true;

	GLuint buffer_object = (GLuint)buffer->backend_data[0];
	if (!buffer_object) {
		glGenBuffers(1, &buffer_object);
		if (rb_gl_check_error("Unable to create buffer object"))
			return false;
		buffer->backend_data[0] = buffer_object;
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer_object);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)buffer->buffersize, buffer->store,
	             (buffer->usage == RENDERUSAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	if (rb_gl_check_error("Unable to upload buffer object data"))
		return false;

	if (buffer->buffertype == RENDERBUFFER_VERTEX) {
		GLuint vertex_array = (GLuint)buffer->backend_data[1];
		if (!vertex_array) {
			glGenVertexArrays(1, &vertex_array);
			if (rb_gl_check_error("Unable to create vertex array"))
				return false;
			buffer->backend_data[1] = vertex_array;
		}
		glBindVertexArray(vertex_array);

		render_vertexbuffer_t* vertexbuffer = (render_vertexbuffer_t*)buffer;
		const render_vertex_decl_t* decl = &vertexbuffer->decl;
		for (unsigned int attrib = 0; attrib < RENDER_MAX_ATTRIBUTES; ++attrib) {
			const uint8_t format = decl->attribute[attrib].format;
			if (format < VERTEXFORMAT_COUNT) {
				glVertexAttribPointer(attrib, rb_gl4_vertex_format_size[format], rb_gl4_vertex_format_type[format],
				                      rb_gl4_vertex_format_norm[format], (GLsizei)decl->attribute[attrib].stride,
				                      (const void*)(uintptr_t)decl->attribute[attrib].offset);
				rb_gl_check_error("Error creating vertex array (bind attribute)");
				glEnableVertexAttribArray(attrib);
				rb_gl_check_error("Error creating vertex array (enable attribute)");
			} else {
				glDisableVertexAttribArray(attrib);
			}
		}
		rb_gl_check_error("Error creating vertex array (bind attributes)");
	}

	buffer->flags &= ~(uint32_t)RENDERBUFFER_DIRTY;

	return true;
}

static void
rb_gl4_link_buffer(render_backend_t* backend, render_buffer_t* buffer, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(program);
}

static bool
rb_gl4_upload_shader(render_backend_t* backend, render_shader_t* shader, const void* buffer, size_t size) {
	bool ret = false;
	// render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;
	FOUNDATION_UNUSED(backend);

	// Shader backend data:
	//  0 - Shader object
	if (shader->backend_data[0])
		glDeleteShader((GLuint)shader->backend_data[0]);

	switch (shader->shadertype) {
		case SHADER_PIXEL:
		case SHADER_VERTEX: {
			bool is_pixel_shader = (shader->shadertype == SHADER_PIXEL);
			GLuint handle = glCreateShader(is_pixel_shader ? GL_FRAGMENT_SHADER_ARB : GL_VERTEX_SHADER_ARB);
			const GLchar* source = (const GLchar*)buffer;
			GLint source_size = (GLint)size;
			glShaderSource(handle, 1, &source, &source_size);
			glCompileShader(handle);

			GLint compiled = 0;
			glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);

			if (!compiled) {
#if BUILD_DEBUG
				GLsizei log_capacity = 2048;
				GLchar* log_buffer = memory_allocate(HASH_RESOURCE, (size_t)log_capacity, 0, MEMORY_TEMPORARY);
				GLint log_length = 0;
				glGetShaderiv(handle, GL_COMPILE_STATUS, &compiled);
				glGetShaderInfoLog(handle, log_capacity, &log_length, log_buffer);
				log_errorf(HASH_RESOURCE, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to compile shader: %.*s"),
				           (int)log_length, log_buffer);
				memory_deallocate(log_buffer);
#endif
				glDeleteShader(handle);
				shader->backend_data[0] = 0;
			} else {
				shader->backend_data[0] = handle;
				ret = true;
			}
		} break;

		default:
			break;
	}
	return ret;
}

static void
rb_gl4_deallocate_shader(render_backend_t* backend, render_shader_t* shader) {
	FOUNDATION_UNUSED(backend);
	if (shader->backend_data[0])
		glDeleteShader((GLuint)shader->backend_data[0]);
	shader->backend_data[0] = 0;
}

static bool
rb_gl4_check_program_link(GLuint handle) {
	GLint result = 0;
	glGetProgramiv(handle, GL_LINK_STATUS, &result);
	if (!result) {
		GLsizei buffer_size = 4096;
		GLint log_length = 0;
		GLchar* log = 0;

		glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &buffer_size);
		log = memory_allocate(HASH_RENDER, (size_t)buffer_size + 1, 0, MEMORY_TEMPORARY);
		glGetProgramInfoLog(handle, buffer_size, &log_length, log);

		log_errorf(ERRORLEVEL_ERROR, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Unable to compile program: %.*s"),
		           (int)log_length, log);
		memory_deallocate(log);

		glDeleteProgram(handle);

		return false;
	}
	return true;
}

static bool
rb_gl4_upload_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	if (program->backend_data[0])
		glDeleteProgram((GLuint)program->backend_data[0]);

	GLint attributes = 0;
	GLint uniforms = 0;
	GLint ia, iu;
	GLsizei char_count = 0;
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
	if (!rb_gl4_check_program_link(handle))
		return false;

	glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &attributes);
	for (ia = 0; ia < attributes; ++ia) {
		char_count = 0;
		size = 0;
		type = GL_NONE;
		glGetActiveAttrib(handle, (GLuint)ia, sizeof(name), &char_count, &size, &type, name);

		name_hash = hash(name, (size_t)char_count);
		for (size_t iattrib = 0; iattrib < program->attributes_count; ++iattrib) {
			if (program->attribute_name[iattrib] == name_hash) {
				glBindAttribLocation(handle, program->attribute[iattrib].binding, name);
				break;
			}
		}
	}
	glLinkProgram(handle);
	if (!rb_gl4_check_program_link(handle))
		return false;

	glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);
	for (iu = 0; iu < uniforms; ++iu) {
		char_count = 0;
		size = 0;
		type = GL_NONE;
		glGetActiveUniform(handle, (GLuint)iu, sizeof(name), &char_count, &size, &type, name);

		name_hash = hash(name, (size_t)char_count);
		for (size_t iparam = 0; iparam < program->parameters_count; ++iparam) {
			render_parameter_t* parameter = program->parameters + iparam;
			if (parameter->name == name_hash)
				parameter->location = (unsigned int)glGetUniformLocation(handle, name);
		}
	}

	program->backend_data[0] = handle;

	return true;
}

static void
rb_gl4_deallocate_program(render_backend_t* backend, render_program_t* program) {
	FOUNDATION_UNUSED(backend);
	if (program->backend_data[0])
		glDeleteProgram((GLuint)program->backend_data[0]);
	program->backend_data[0] = 0;
}

bool
rb_gl_allocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (!target->width || !target->height)
		return false;

	GLuint frame_buffer = 0;
	GLuint depth_buffer = 0;
	GLuint render_texture = 0;
	GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
	GLenum status;

	memset(target->backend_data, 0, sizeof(target->backend_data));

	glGenFramebuffers(1, &frame_buffer);
	if (!frame_buffer) {
		if (!rb_gl_check_error("Unable to create render target: Error creating frame buffer")) {
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to create render target: Error creating frame buffer (no error)"));
			goto failure;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	if (rb_gl_check_error("Unable to create render target: Error binding framebuffer"))
		goto failure;

	glGenTextures(1, &render_texture);
	if (!render_texture) {
		if (!rb_gl_check_error("Unable to create render target: Error creating texture"))
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to create render target: Error creating texture (no error)"));
		goto failure;
	}
	glBindTexture(GL_TEXTURE_2D, render_texture);

	GLenum glformat = GL_RGB;
	GLint internalformat = (target->colorspace == COLORSPACE_sRGB) ? GL_SRGB_EXT : GL_RGB;
	if (target->pixelformat == PIXELFORMAT_R8G8B8A8) {
		glformat = GL_RGBA;
		internalformat = (target->colorspace == COLORSPACE_sRGB) ? GL_SRGB8_ALPHA8_EXT : GL_RGBA;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, (GLsizei)target->width, (GLsizei)target->height, 0, glformat,
	             GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (rb_gl_check_error("Unable to create render target: Error setting texture storage dimensions and format"))
		goto failure;
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenRenderbuffers(1, &depth_buffer);
	if (!depth_buffer) {
		if (!rb_gl_check_error("Unable to create render target: Error creating depth buffer"))
			log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
			           STRING_CONST("Unable to create render target: Error creating depth buffer (no error)"));
		goto failure;
	}
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)target->width, (GLsizei)target->height);
	if (rb_gl_check_error("Unable to create render target: Error setting depth buffer storage dimensions"))
		goto failure;
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

#if FOUNDATION_PLATFORM_MACOS
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture, 0);
#else
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);
#endif
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	if (rb_gl_check_error("Unable to create render target: Error setting target attachments"))
		goto failure;

	glDrawBuffers(1, &draw_buffers);

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to create render target: Frame buffer not complete (%d)"), (int)status);
		goto failure;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	target->backend_data[0] = frame_buffer;
	target->backend_data[1] = depth_buffer;
	target->backend_data[2] = render_texture;

	return true;

failure:

	if (render_texture)
		glDeleteTextures(1, &render_texture);
	if (depth_buffer)
		glDeleteRenderbuffers(1, &depth_buffer);
	if (frame_buffer)
		glDeleteFramebuffers(1, &frame_buffer);

	return false;
}

bool
rb_gl_resize_target(render_backend_t* backend, render_target_t* target, unsigned int width, unsigned int height) {
	if (!target->backend_data[0]) {
		// If the frame buffer was resized, update the drawable info as well
		backend->drawable.width = width;
		backend->drawable.height = height;
		backend->framebuffer.width = width;
		backend->framebuffer.height = height;
		return true;
	}

	GLuint frame_buffer = (GLuint)target->backend_data[0];
	GLuint depth_buffer = (GLuint)target->backend_data[1];
	GLuint render_texture = (GLuint)target->backend_data[2];
	GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
	GLenum status;

	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	if (rb_gl_check_error("Unable to resize render target: Error binding framebuffer")) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, render_texture);

	GLenum glformat = GL_RGB;
	GLint internalformat = (target->colorspace == COLORSPACE_sRGB) ? GL_SRGB_EXT : GL_RGB;
	if (target->pixelformat == PIXELFORMAT_R8G8B8A8) {
		glformat = GL_RGBA;
		internalformat = (target->colorspace == COLORSPACE_sRGB) ? GL_SRGB8_ALPHA8_EXT : GL_RGBA;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, (GLsizei)width, (GLsizei)height, 0, glformat, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (rb_gl_check_error("Unable to resize render target: Error setting texture storage "
	                      "dimensions and format")) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (GLsizei)width, (GLsizei)height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	if (rb_gl_check_error("Unable to resize render target: Error setting depth buffer storage dimensions")) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

#if FOUNDATION_PLATFORM_MACOS
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture, 0);
#else
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);
#endif
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	if (rb_gl_check_error("Unable to resize render target: Error setting target attachments")) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glDrawBuffers(1, &draw_buffers);

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		log_errorf(HASH_RENDER, ERROR_SYSTEM_CALL_FAIL,
		           STRING_CONST("Unable to create render target: Frame buffer not complete (%d)"), (int)status);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void
rb_gl_deallocate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	if (target->backend_data[2])
		glDeleteTextures(1, (const GLuint*)&target->backend_data[2]);
	if (target->backend_data[1])
		glDeleteRenderbuffers(1, (const GLuint*)&target->backend_data[1]);
	if (target->backend_data[0])
		glDeleteFramebuffers(1, (const GLuint*)&target->backend_data[0]);
	memset(target->backend_data, 0, sizeof(target->backend_data));
}

bool
rb_gl_activate_target(render_backend_t* backend, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)target->backend_data[0]);
	return true;
}

bool
rb_gl_upload_texture(render_backend_t* backend, render_texture_t* texture, const void* buffer, size_t size) {
	FOUNDATION_UNUSED(backend);
	FOUNDATION_UNUSED(size);

	rb_gl_check_error("Error prior to texture upload");

	glActiveTexture(GL_TEXTURE0);
	GLuint texture_name = (GLuint)texture->backend_data[0];
	if (!texture_name) {
		glGenTextures(1, &texture_name);
		if (!texture_name) {
			rb_gl_check_error("Unable to create texture object");
			return false;
		}
		texture->backend_data[0] = texture_name;
	}

	glBindTexture(GL_TEXTURE_2D, texture_name);

	bool generate_mipmaps = !!(texture->textureflags & RENDERTEXTURE_FLAG_AUTOGENERATE_MIPMAPS);
	bool has_mipmaps = (texture->levels > 1) || generate_mipmaps;
	if (backend->api == RENDERAPI_OPENGL2)
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, generate_mipmaps ? GL_TRUE : GL_FALSE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, has_mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (rb_gl_check_error("Unable to initialize texture data"))
		return false;

	bool compressed = false;
	GLenum internal_format = GL_RGBA;
	GLenum data_format = GL_RGBA;
	GLenum data_type = GL_UNSIGNED_BYTE;
	unsigned int bits_per_pixel = 0;

	switch (texture->pixelformat) {
		case PIXELFORMAT_R8G8B8:
			internal_format = GL_RGB;
			data_format = GL_RGB;
			data_type = GL_UNSIGNED_BYTE;
			bits_per_pixel = 24;
			if (texture->colorspace == COLORSPACE_sRGB)
				internal_format = GL_SRGB;
			break;
		case PIXELFORMAT_R8G8B8A8:
			internal_format = GL_RGBA;
			data_format = GL_RGBA;
			data_type = GL_UNSIGNED_BYTE;
			bits_per_pixel = 32;
			if (texture->colorspace == COLORSPACE_sRGB)
				internal_format = GL_SRGB_ALPHA;
			break;

		case PIXELFORMAT_R16G16B16:
			internal_format = GL_RGB;
			data_format = GL_RGB;
			data_type = GL_UNSIGNED_SHORT;
			bits_per_pixel = 48;
			if (texture->colorspace == COLORSPACE_sRGB)
				internal_format = GL_SRGB;
			break;
		case PIXELFORMAT_R16G16B16A16:
			internal_format = GL_RGBA;
			data_format = GL_RGBA;
			data_type = GL_UNSIGNED_SHORT;
			bits_per_pixel = 64;
			if (texture->colorspace == COLORSPACE_sRGB)
				internal_format = GL_SRGB_ALPHA;
			break;

		case PIXELFORMAT_R32G32B32F:
			internal_format = GL_RGB;
			data_format = GL_RGB;
			data_type = GL_FLOAT;
			bits_per_pixel = 96;
			break;
		case PIXELFORMAT_R32G32B32A32F:
			internal_format = GL_RGBA;
			data_format = GL_RGBA;
			data_type = GL_FLOAT;
			bits_per_pixel = 128;
			break;

		case PIXELFORMAT_A8:
			internal_format = GL_ALPHA;
			data_format = GL_ALPHA;
			data_type = GL_UNSIGNED_BYTE;
			break;

		case PIXELFORMAT_PVRTC_2:
		case PIXELFORMAT_PVRTC_4:
			// TODO: Implement
			break;

		case PIXELFORMAT_INVALID:
		case PIXELFORMAT_COUNT:
		default:
			break;
	}
	const void* data = buffer;
	unsigned int level_width = texture->width;
	unsigned int level_height = texture->height;
	for (GLint ilevel = 0; ilevel < (GLint)texture->levels; ++ilevel) {
		unsigned int data_length = level_width * level_height * (bits_per_pixel / 8);
		if (data_length > size) {
			log_error(HASH_RENDER, ERROR_INVALID_VALUE, STRING_CONST("Data size too small"));
			return false;
		}
		if (!compressed)
			glTexImage2D(GL_TEXTURE_2D, ilevel, (GLint)internal_format, (GLsizei)level_width, (GLsizei)level_height, 0,
			             data_format, data_type, data);
		else
			glCompressedTexImage2D(GL_TEXTURE_2D, ilevel, internal_format, (GLsizei)level_width, (GLsizei)level_height,
			                       0, (GLsizei)data_length, data);

		data = pointer_offset_const(data, data_length);
		size -= data_length;
		level_width = math_max(level_width >> 1, 1);
		level_height = math_max(level_height >> 1, 1);
	}

	if (generate_mipmaps && (backend->api != RENDERAPI_OPENGL2))
		glGenerateMipmap(GL_TEXTURE_2D);

	if (rb_gl_check_error("Unable to upload texture data"))
		return false;

	return true;
}

void
rb_gl_parameter_bind_texture(render_backend_t* backend, void* buffer, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	*(GLuint*)buffer = (GLuint)texture->backend_data[0];
}

void
rb_gl_parameter_bind_target(render_backend_t* backend, void* buffer, render_target_t* target) {
	FOUNDATION_UNUSED(backend);
	*(GLuint*)buffer = (GLuint)target->backend_data[2];
}

void
rb_gl_deallocate_texture(render_backend_t* backend, render_texture_t* texture) {
	FOUNDATION_UNUSED(backend);
	if (texture->backend_data[0])
		glDeleteTextures(1, (GLuint*)&texture->backend_data[0]);
	texture->backend_data[0] = 0;
}

static void
rb_gl4_clear(render_backend_gl4_t* backend, render_context_t* context, render_command_t* command) {
	unsigned int buffer_mask = command->data.clear.buffer_mask;
	unsigned int bits = 0;
	FOUNDATION_UNUSED(context);

	if (buffer_mask & RENDERBUFFER_COLOR) {
		unsigned int color_mask = command->data.clear.color_mask;
		uint32_t color = command->data.clear.color;
		glColorMask((color_mask & 0x01) ? GL_TRUE : GL_FALSE, (color_mask & 0x02) ? GL_TRUE : GL_FALSE,
		            (color_mask & 0x04) ? GL_TRUE : GL_FALSE, (color_mask & 0x08) ? GL_TRUE : GL_FALSE);
		bits |= GL_COLOR_BUFFER_BIT;
		// color_linear_t color = uint32_to_color( command->data.clear.color );
		// glClearColor( vector_x( color ), vector_y( color ), vector_z( color ), vector_w( color )
		// );
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

	rb_gl_check_error("Error clearing targets");
}

static void
rb_gl4_viewport(render_backend_gl4_t* backend, render_target_t* target, render_context_t* context,
                render_command_t* command) {
	FOUNDATION_UNUSED(context);
	GLint x = (GLint)command->data.viewport.x;
	GLint y = (GLint)command->data.viewport.y;
	GLsizei w = (GLsizei)command->data.viewport.width;
	GLsizei h = (GLsizei)command->data.viewport.height;

	glViewport(x, y, w, h);
	glScissor(x, y, w, h);

	backend->use_clear_scissor = (x || y || (w != (GLsizei)target->width) || (h != (GLsizei)target->height));

	rb_gl_check_error("Error setting viewport");
}

static const GLenum rb_gl4_primitive_type[] = {GL_TRIANGLES, GL_LINES};
static const unsigned int rb_gl4_primitive_mult[] = {3, 2};
static const unsigned int rb_gl4_primitive_add[] = {0, 0};

static const GLenum rb_gl4_index_format_type[INDEXFORMAT_COUNT] = {GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT,
                                                                   GL_UNSIGNED_INT};

static const GLenum rb_gl4_blend_func[] = {GL_ZERO,
                                           GL_ONE,
                                           GL_SRC_COLOR,
                                           GL_ONE_MINUS_SRC_COLOR,
                                           GL_DST_COLOR,
                                           GL_ONE_MINUS_DST_COLOR,
                                           GL_SRC_ALPHA,
                                           GL_ONE_MINUS_SRC_ALPHA,
                                           GL_DST_ALPHA,
                                           GL_ONE_MINUS_DST_ALPHA,
                                           GL_CONSTANT_ALPHA,
                                           GL_ONE_MINUS_CONSTANT_ALPHA,
                                           GL_SRC_ALPHA_SATURATE};

static const GLenum rb_gl4_cmp_func[] = {GL_NEVER,    GL_LESS,   GL_LEQUAL,  GL_EQUAL,
                                         GL_NOTEQUAL, GL_GEQUAL, GL_GREATER, GL_ALWAYS};

static void
rb_gl4_set_default_state(void) {
	glBlendFunc(GL_ONE, GL_ZERO);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
}

static void
rb_gl4_set_state(render_state_t* state) {
	glBlendFunc(rb_gl4_blend_func[state->blend_source_color], rb_gl4_blend_func[state->blend_dest_color]);
	if (state->blend_enable[0])
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
	glDepthFunc(rb_gl4_cmp_func[state->depth_func]);
	glDepthMask(state->depth_write ? GL_TRUE : GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	glFrontFace((state->cull_mode == RENDER_CULL_CCW) ? GL_CW : GL_CCW);
	if (state->cull_mode != RENDER_CULL_NONE)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	glLineWidth(2.0f);
}

static void
rb_gl4_render(render_backend_gl4_t* backend, render_context_t* context, render_command_t* command) {
	render_vertexbuffer_t* vertexbuffer = command->data.render.vertexbuffer;
	render_indexbuffer_t* indexbuffer = command->data.render.indexbuffer;
	render_parameterbuffer_t* parameterbuffer = command->data.render.parameterbuffer;
	render_program_t* program = command->data.render.program;
	FOUNDATION_UNUSED(context);

	if (!vertexbuffer || !indexbuffer || !parameterbuffer || !program) {  // Outdated references
		FOUNDATION_ASSERT_FAIL("Render command using invalid resources");
		return;
	}

	if (vertexbuffer->flags & RENDERBUFFER_DIRTY)
		rb_gl4_upload_buffer((render_backend_t*)backend, (render_buffer_t*)vertexbuffer);
	if (indexbuffer->flags & RENDERBUFFER_DIRTY)
		rb_gl4_upload_buffer((render_backend_t*)backend, (render_buffer_t*)indexbuffer);
	rb_gl_check_error("Error render primitives (upload buffers)");

	// Bind vertex array
	GLuint vertex_array = (GLuint)vertexbuffer->backend_data[1];
	glBindVertexArray(vertex_array);
	rb_gl_check_error("Error render primitives (bind vertex array)");

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)indexbuffer->backend_data[0]);
	rb_gl_check_error("Error render primitives (bind index buffer)");

	// Bind programs/shaders
	glUseProgram((GLuint)program->backend_data[0]);
	rb_gl_check_error("Error render primitives (bind program)");

	// Bind the parameter blocks
	GLuint unit = 0;
	render_parameter_t* param = parameterbuffer->parameters;
	for (unsigned int ip = 0; ip < parameterbuffer->parameter_count; ++ip, ++param) {
		void* data = pointer_offset(parameterbuffer->store, param->offset);
		if (param->type == RENDERPARAMETER_TEXTURE) {
			glActiveTexture(GL_TEXTURE0 + unit);
			// glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, *(GLuint*)data);
			glUniform1i((GLint)param->location, (GLint)unit);
			++unit;
		} else if (param->type == RENDERPARAMETER_FLOAT4) {
			glUniform4fv((GLint)param->location, param->dim, data);
		} else if (param->type == RENDERPARAMETER_INT4) {
			glUniform4iv((GLint)param->location, param->dim, data);
		} else if (param->type == RENDERPARAMETER_MATRIX) {
			// Matrix math is row-major, must be transposed to match GL layout which is column major
			glUniformMatrix4fv((GLint)param->location, param->dim, GL_TRUE, data);
		}
	}
	rb_gl_check_error("Error render primitives (bind uniforms)");

	// TODO: Proper states
	/*ID3D10Device_RSSetState( device, backend_dx10->rasterizer_state[0].state );

	FLOAT blend_factors[] = { 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f };
	ID3D10Device_OMSetBlendState( device, backend_dx10->blend_state[ (
	command->data.render.blend_state >> 48ULL ) & 0xFFFFULL ].state, blend_factors, 0xFFFFFFFF );
	ID3D10Device_OMSetDepthStencilState( device, backend_dx10->depthstencil_state[0].state,
	0xFFFFFFFF );*/

	if (command->data.render.statebuffer) {
		// Set state from buffer
		render_statebuffer_t* buffer = command->data.render.statebuffer;
		rb_gl4_set_state(&buffer->state);
	} else {
		// Set default state
		rb_gl4_set_default_state();
	}

	unsigned int primitive = command->type - RENDERCOMMAND_RENDER_TRIANGLELIST;
	unsigned int count = command->count;
	unsigned int gl_count = rb_gl4_primitive_mult[primitive] * count + rb_gl4_primitive_add[primitive];

	glDrawElements(rb_gl4_primitive_type[primitive], (GLsizei)gl_count, rb_gl4_index_format_type[indexbuffer->format],
	               0);

	rb_gl_check_error("Error render primitives");
}

static void
rb_gl4_dispatch_command(render_backend_gl4_t* backend, render_target_t* target, render_context_t* context,
                        render_command_t* command) {
	switch (command->type) {
		case RENDERCOMMAND_CLEAR:
			rb_gl4_clear(backend, context, command);
			break;

		case RENDERCOMMAND_VIEWPORT:
			rb_gl4_viewport(backend, target, context, command);
			break;

		case RENDERCOMMAND_RENDER_TRIANGLELIST:
		case RENDERCOMMAND_RENDER_LINELIST:
			rb_gl4_render(backend, context, command);
			break;
	}
}

static void
rb_gl4_dispatch(render_backend_t* backend, render_target_t* target, render_context_t** contexts,
                size_t contexts_count) {
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

	if (!rb_gl_activate_target(backend, target))
		return;

	for (size_t cindex = 0, csize = contexts_count; cindex < csize; ++cindex) {
		render_context_t* context = contexts[cindex];
		int cmd_size = atomic_load32(&context->reserved, memory_order_acquire);
		if (context->sort->indextype == RADIXSORT_INDEX16) {
			const uint16_t* order = context->order;
			for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order)
				rb_gl4_dispatch_command(backend_gl4, target, context, context->commands + *order);
		} else if (context->sort->indextype == RADIXSORT_INDEX32) {
			const uint32_t* order = context->order;
			for (int cmd_index = 0; cmd_index < cmd_size; ++cmd_index, ++order)
				rb_gl4_dispatch_command(backend_gl4, target, context, context->commands + *order);
		}
	}
}

static void
rb_gl4_flip(render_backend_t* backend) {
	render_backend_gl4_t* backend_gl4 = (render_backend_gl4_t*)backend;

#if FOUNDATION_PLATFORM_WINDOWS

	if (backend_gl4->drawable.hdc) {
		HDC current_dc = wglGetCurrentDC();
		FOUNDATION_ASSERT(current_dc == backend_gl4->drawable.hdc);
		if (!SwapBuffers(backend_gl4->drawable.hdc)) {
			string_const_t errmsg = system_error_message(0);
			log_warnf(HASH_RENDER, WARNING_SYSTEM_CALL_FAIL, STRING_CONST("SwapBuffers failed: %.*s"),
			          STRING_FORMAT(errmsg));
		}
	}

#elif FOUNDATION_PLATFORM_MACOS

	if (backend_gl4->context) {
		/*if( backend_gl2->fullscreen )
		    CGLFlushDrawable( backend_gl2->context );
		else*/
		rb_gl_flush_drawable(backend_gl4->context);
	}

#elif FOUNDATION_PLATFORM_LINUX

	if (backend_gl4->drawable.display)
		glXSwapBuffers(backend_gl4->drawable.display, (GLXDrawable)backend_gl4->drawable.drawable);

#else
#error Not implemented
#endif

	++backend->framecount;
}

static render_backend_vtable_t render_backend_vtable_gl4 = {.construct = rb_gl4_construct,
                                                            .destruct = rb_gl4_destruct,
                                                            .enumerate_adapters = rb_gl_enumerate_adapters,
                                                            .enumerate_modes = rb_gl_enumerate_modes,
                                                            .set_drawable = rb_gl4_set_drawable,
                                                            .enable_thread = rb_gl4_enable_thread,
                                                            .disable_thread = rb_gl4_disable_thread,
                                                            .allocate_buffer = rb_gl4_allocate_buffer,
                                                            .upload_buffer = rb_gl4_upload_buffer,
                                                            .upload_shader = rb_gl4_upload_shader,
                                                            .upload_program = rb_gl4_upload_program,
                                                            .upload_texture = rb_gl_upload_texture,
                                                            .parameter_bind_texture = rb_gl_parameter_bind_texture,
                                                            .parameter_bind_target = rb_gl_parameter_bind_target,
                                                            .link_buffer = rb_gl4_link_buffer,
                                                            .deallocate_buffer = rb_gl4_deallocate_buffer,
                                                            .deallocate_shader = rb_gl4_deallocate_shader,
                                                            .deallocate_program = rb_gl4_deallocate_program,
                                                            .deallocate_texture = rb_gl_deallocate_texture,
                                                            .allocate_target = rb_gl_allocate_target,
                                                            .resize_target = rb_gl_resize_target,
                                                            .deallocate_target = rb_gl_deallocate_target,
                                                            .dispatch = rb_gl4_dispatch,
                                                            .flip = rb_gl4_flip};

render_backend_t*
render_backend_gl4_allocate(void) {
	rb_gl_init_gl_extensions();
	if (!rb_gl_check_context(4, 0))
		return 0;

#if RENDER_ENABLE_NVGLEXPERT
	static bool nvInitialized = false;
	if (!nvInitialized) {
		nvInitialized = true;
		NvAPI_Initialize();
	}
#endif

	render_backend_gl4_t* backend =
	    memory_allocate(HASH_RENDER, sizeof(render_backend_gl4_t), 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	backend->api = RENDERAPI_OPENGL4;
	backend->api_group = RENDERAPIGROUP_OPENGL;
	backend->vtable = render_backend_vtable_gl4;
	return (render_backend_t*)backend;
}

#else

render_backend_t*
render_backend_gl4_allocate(void) {
	return 0;
}

#endif

#if FOUNDATION_COMPILER_CLANG
#pragma clang diagnostic pop
#endif
