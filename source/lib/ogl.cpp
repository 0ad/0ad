/* Copyright (C) 2022 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"

#include "lib/ogl.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"
#include "lib/debug.h"
#include "lib/external_libraries/libsdl.h"
#include "ps/CLogger.h"

#if !CONFIG2_GLES

# if OS_WIN
#  include <glad/wgl.h>
# elif !OS_MACOSX && !OS_MAC
#  include <SDL_syswm.h>
#  if defined(SDL_VIDEO_DRIVER_X11)
#   include <glad/glx.h>
#  endif
#  if defined(SDL_VIDEO_DRIVER_WAYLAND)
#   include <glad/egl.h>
#  endif
# endif

#endif // !CONFIG2_GLES

#include <stdio.h>
#include <string.h>
#include <stdarg.h>


//----------------------------------------------------------------------------
// extensions
//----------------------------------------------------------------------------

static const char* exts = nullptr;

static bool have_30 = false;
static bool have_21 = false;
static bool have_20 = false;
static bool have_15 = false;
static bool have_14 = false;
static bool have_13 = false;
static bool have_12 = false;


// return a C string of unspecified length containing a space-separated
// list of all extensions the OpenGL implementation advertises.
// (useful for crash logs).
const char* ogl_ExtensionString()
{
	ENSURE(exts && "call ogl_Init before using this function");
	return exts;
}


// paranoia: newer drivers may forget to advertise an extension
// indicating support for something that has been folded into the core.
// we therefore check for all extensions known to be offered by the
// GL implementation present on the user's system; ogl_HaveExtension will
// take this into account.
// the app can therefore just ask for extensions and not worry about this.
static bool isImplementedInCore(const char* ext)
{
#define MATCH(known_ext)\
	if(!strcmp(ext, #known_ext))\
		return true;

	if(have_30)
	{
		MATCH(GL_EXT_gpu_shader4);
		MATCH(GL_NV_conditional_render);
		MATCH(GL_ARB_color_buffer_float);
		MATCH(GL_ARB_depth_buffer_float);
		MATCH(GL_ARB_texture_float);
		MATCH(GL_EXT_packed_float);
		MATCH(GL_EXT_texture_shared_exponent);
		MATCH(GL_EXT_framebuffer_object);
		MATCH(GL_NV_half_float);
		MATCH(GL_ARB_half_float_pixel);
		MATCH(GL_EXT_framebuffer_multisample);
		MATCH(GL_EXT_framebuffer_blit);
		MATCH(GL_EXT_texture_integer);
		MATCH(GL_EXT_texture_array);
		MATCH(GL_EXT_packed_depth_stencil);
		MATCH(GL_EXT_draw_buffers2);
		MATCH(GL_EXT_texture_compression_rgtc);
		MATCH(GL_EXT_transform_feedback);
		MATCH(GL_APPLE_vertex_array_object);
		MATCH(GL_EXT_framebuffer_sRGB);
	}
	if(have_21)
	{
		MATCH(GL_ARB_pixel_buffer_object);
		MATCH(GL_EXT_texture_sRGB);
	}
	if(have_20)
	{
		MATCH(GL_ARB_shader_objects);
		MATCH(GL_ARB_vertex_shader);
		MATCH(GL_ARB_fragment_shader);
		MATCH(GL_ARB_shading_language_100);
		MATCH(GL_ARB_draw_buffers);
		MATCH(GL_ARB_texture_non_power_of_two);
		MATCH(GL_ARB_point_sprite);
		MATCH(GL_EXT_blend_equation_separate);
	}
	if(have_15)
	{
		MATCH(GL_ARB_vertex_buffer_object);
		MATCH(GL_ARB_occlusion_query);
		MATCH(GL_EXT_shadow_funcs);
	}
	if(have_14)
	{
		MATCH(GL_SGIS_generate_mipmap);
		MATCH(GL_NV_blend_square);
		MATCH(GL_ARB_depth_texture);
		MATCH(GL_ARB_shadow);
		MATCH(GL_EXT_fog_coord);
		MATCH(GL_EXT_multi_draw_arrays);
		MATCH(GL_ARB_point_parameters);
		MATCH(GL_EXT_secondary_color);
		MATCH(GL_EXT_blend_func_separate);
		MATCH(GL_EXT_stencil_wrap);
		MATCH(GL_ARB_texture_env_crossbar);
		MATCH(GL_EXT_texture_lod_bias);
		MATCH(GL_ARB_texture_mirrored_repeat);
		MATCH(GL_ARB_window_pos);

		// These extensions were added to GL 1.2, but as part of the optional
		// imaging subset; they're only guaranteed as of GL 1.4:
		MATCH(GL_EXT_blend_color);
		MATCH(GL_EXT_blend_minmax);
		MATCH(GL_EXT_blend_subtract);
	}
	if(have_13)
	{
		MATCH(GL_ARB_texture_compression);
		MATCH(GL_ARB_texture_cube_map);
		MATCH(GL_ARB_multisample);
		MATCH(GL_ARB_multitexture);
		MATCH(GL_ARB_transpose_matrix);
		MATCH(GL_ARB_texture_env_add);
		MATCH(GL_ARB_texture_env_combine);
		MATCH(GL_ARB_texture_env_dot3);
		MATCH(GL_ARB_texture_border_clamp);
	}
	if(have_12)
	{
		MATCH(GL_EXT_texture3D);
		MATCH(GL_EXT_bgra);
		MATCH(GL_EXT_packed_pixels);
		MATCH(GL_EXT_rescale_normal);
		MATCH(GL_EXT_separate_specular_color);
		MATCH(GL_SGIS_texture_edge_clamp);
		MATCH(GL_SGIS_texture_lod);
		MATCH(GL_EXT_draw_range_elements);
		// Skip the extensions that only affect the imaging subset
	}

#undef MATCH
	return false;
}


// check if the extension <ext> is supported by the OpenGL implementation.
// takes subsequently added core support for some extensions into account.
bool ogl_HaveExtension(const char* ext)
{
	ENSURE(exts && "call ogl_Init before using this function");

	if(isImplementedInCore(ext))
		return true;

	const char *p = exts, *end;

	// make sure ext is valid & doesn't contain spaces
	if(!ext || ext[0] == '\0' || strchr(ext, ' '))
		return false;

	for(;;)
	{
		p = strstr(p, ext);
		if(!p)
			return false; // <ext> string not found - extension not supported
		end = p + strlen(ext); // end of current substring

		// make sure the substring found is an entire extension string,
		// i.e. it starts and ends with ' '
		if((p == exts || p[-1] == ' ') &&	// valid start AND
		   (*end == ' ' || *end == '\0'))	// valid end
			return true;
		p = end;
	}
}

static int GLVersion;
#if OS_WIN
static int WGLVersion;
#elif !CONFIG2_GLES && !OS_MACOSX && !OS_MAC
#if defined(SDL_VIDEO_DRIVER_X11)
static int GLXVersion;
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
static int EGLVersion;
#endif
#endif

bool ogl_HaveVersion(int major, int minor)
{
	return GLAD_MAKE_VERSION(major, minor) <= GLVersion;
}


// check if all given extension strings (passed as const char* parameters,
// terminated by a 0 pointer) are supported by the OpenGL implementation,
// as determined by ogl_HaveExtension.
// returns 0 if all are present; otherwise, the first extension in the
// list that's not supported (useful for reporting errors).
//
// note: dummy parameter is necessary to access parameter va_list.
//
//
// rationale: this interface is more convenient than individual
// ogl_HaveExtension calls and allows reporting which extension is missing.
//
// one disadvantage is that there is no way to indicate that either one
// of 2 extensions would be acceptable, e.g. (ARB|EXT)_texture_env_dot3.
// this is isn't so bad, since they wouldn't be named differently
// if there weren't non-trivial changes between them. for that reason,
// we refrain from equivalence checks (which would boil down to
// string-matching known extensions to their equivalents).
const char* ogl_HaveExtensions(int dummy, ...)
{
	const char* ext;

	va_list args;
	va_start(args, dummy);
	for(;;)
	{
		ext = va_arg(args, const char*);
		// end of list reached; all were present => return 0.
		if(!ext)
			break;

		// not found => return name of missing extension.
		if(!ogl_HaveExtension(ext))
			break;
	}
	va_end(args);

	return ext;
}


// to help when running with no hardware acceleration and only OpenGL 1.1
// (e.g. testing the game in virtual machines), we define dummy versions of
// some extension functions which our graphics code assumes exist.
// it will render incorrectly but at least it shouldn't crash.

#if CONFIG2_GLES

static void enableDummyFunctions()
{
}

#else

static void GLAD_API_PTR dummy_glDrawRangeElementsEXT(GLenum mode, GLuint, GLuint, GLsizei count, GLenum type, GLvoid* indices)
{
	glDrawElements(mode, count, type, indices);
}

static void GLAD_API_PTR dummy_glActiveTextureARB(GLenum UNUSED(texture))
{
}

static void GLAD_API_PTR dummy_glClientActiveTextureARB(GLenum UNUSED(texture))
{
}

static void GLAD_API_PTR dummy_glMultiTexCoord2fARB(GLenum UNUSED(target), GLfloat s, GLfloat t)
{
	glTexCoord2f(s, t);
}

static void GLAD_API_PTR dummy_glMultiTexCoord3fARB(GLenum UNUSED(target), GLfloat s, GLfloat t, GLfloat r)
{
	glTexCoord3f(s, t, r);
}

static void enableDummyFunctions()
{
	// fall back to the dummy functions when extensions (or equivalent core support) are missing

	if(!ogl_HaveExtension("GL_EXT_draw_range_elements"))
	{
		glDrawRangeElementsEXT = reinterpret_cast<PFNGLDRAWRANGEELEMENTSEXTPROC>(&dummy_glDrawRangeElementsEXT);
	}

	if(!ogl_HaveExtension("GL_ARB_multitexture"))
	{
		glActiveTextureARB = reinterpret_cast<PFNGLACTIVETEXTUREARBPROC>(&dummy_glActiveTextureARB);
		glClientActiveTextureARB = reinterpret_cast<PFNGLACTIVETEXTUREARBPROC>(&dummy_glClientActiveTextureARB);
		glMultiTexCoord2fARB = reinterpret_cast<PFNGLMULTITEXCOORD2FARBPROC>(&dummy_glMultiTexCoord2fARB);
		glMultiTexCoord3fARB = reinterpret_cast<PFNGLMULTITEXCOORD3FARBPROC>(&dummy_glMultiTexCoord3fARB);
	}
}

#endif	// #if CONFIG2_GLES

//----------------------------------------------------------------------------

const char* ogl_GetErrorName(GLenum err)
{
#define E(e) case e: return #e;
	switch (err)
	{
	E(GL_INVALID_ENUM)
	E(GL_INVALID_VALUE)
	E(GL_INVALID_OPERATION)
#if !CONFIG2_GLES
	E(GL_STACK_OVERFLOW)
	E(GL_STACK_UNDERFLOW)
#endif
	E(GL_OUT_OF_MEMORY)
	E(GL_INVALID_FRAMEBUFFER_OPERATION)
	default: return "Unknown GL error";
	}
#undef E
}

static void dump_gl_error(GLenum err)
{
	debug_printf("OGL| %s (%04x)\n", ogl_GetErrorName(err), err);
}

void ogl_WarnIfErrorLoc(const char *file, int line)
{
	// glGetError may return multiple errors, so we poll it in a loop.
	// the debug_printf should only happen once (if this is set), though.
	bool error_enountered = false;
	GLenum first_error = 0;

	for(;;)
	{
		GLenum err = glGetError();
		if(err == GL_NO_ERROR)
			break;

		if(!error_enountered)
			first_error = err;

		error_enountered = true;
		dump_gl_error(err);
	}

	if(error_enountered)
		debug_printf("%s:%d: OpenGL error(s) occurred: %s (%04x)\n", file, line, ogl_GetErrorName(first_error), (unsigned int)first_error);
}

// ignore and reset the specified error (as returned by glGetError).
// any other errors that have occurred are reported as ogl_WarnIfError would.
//
// this is useful for suppressing annoying error messages, e.g.
// "invalid enum" for GL_CLAMP_TO_EDGE even though we've already
// warned the user that their OpenGL implementation is too old.
bool ogl_SquelchError(GLenum err_to_ignore)
{
	// glGetError may return multiple errors, so we poll it in a loop.
	// the debug_printf should only happen once (if this is set), though.
	bool error_enountered = false;
	bool error_ignored = false;
	GLenum first_error = 0;

	for(;;)
	{
		GLenum err = glGetError();
		if(err == GL_NO_ERROR)
			break;

		if(err == err_to_ignore)
		{
			error_ignored = true;
			continue;
		}

		if(!error_enountered)
			first_error = err;

		error_enountered = true;
		dump_gl_error(err);
	}

	if(error_enountered)
		debug_printf("OpenGL error(s) occurred: %04x\n", (unsigned int)first_error);

	return error_ignored;
}


//----------------------------------------------------------------------------
// feature and limit detect
//----------------------------------------------------------------------------

#if OS_WIN
bool ogl_Init(void* (load)(const char*), void* hdc)
#elif !CONFIG2_GLES && !OS_MACOSX && !OS_MAC
bool ogl_Init(void* (load)(const char*), void* display, int subsystem)
#else
bool ogl_Init(void* (load)(const char*))
#endif
{
	GLADloadfunc loadFunc = reinterpret_cast<GLADloadfunc>(load);
	if (!loadFunc)
		return false;

#define LOAD_ERROR(ERROR_STRING) \
	if (g_Logger) \
		LOGERROR(ERROR_STRING); \
	else \
		debug_printf(ERROR_STRING); \

#if !CONFIG2_GLES
	GLVersion = gladLoadGL(loadFunc);
	if (!GLVersion)
	{
		LOAD_ERROR("Failed to load OpenGL functions.");
		return false;
	}
# if OS_WIN
	WGLVersion = gladLoadWGL(reinterpret_cast<HDC>(hdc), loadFunc);
	if (!WGLVersion)
	{
		LOAD_ERROR("Failed to load WGL functions.");
		return false;
	}
# elif !OS_MACOSX && !OS_MAC
	const SDL_SYSWM_TYPE sysWMType = static_cast<SDL_SYSWM_TYPE>(subsystem);
#  if defined(SDL_VIDEO_DRIVER_X11)
	if (sysWMType == SDL_SYSWM_X11)
	{
		GLXVersion = gladLoadGLX(reinterpret_cast<Display*>(display), DefaultScreen(display), loadFunc);
		if (!GLXVersion)
		{
			LOAD_ERROR("Failed to load GLX functions.");
			return false;
		}
	}
#  endif
#  if defined(SDL_VIDEO_DRIVER_WAYLAND)
	if (sysWMType == SDL_SYSWM_WAYLAND)
	{
		// TODO: investiage do we need Wayland display to load EGL.
		// Because without eglGetDisplay we can't get one. But the
		// function is loaded inside gladLoadEGL. So maybe we need to
		// call it twice.
		EGLVersion = gladLoadEGL(nullptr, loadFunc);
		if (!EGLVersion)
		{
			LOAD_ERROR("Failed to load EGL functions.");
			return false;
		}
	}
#  endif
# endif
#else
	GLVersion = gladLoadGLES2(loadFunc);
	if (!GLVersion)
	{
		LOAD_ERROR("Failed to load GLES2 functions.");
		return false;
	}
#endif
#undef LOAD_ERROR

	// cache extension list and versions for oglHave*.
	// note: this is less about performance (since the above are not
	// time-critical) than centralizing the 'OpenGL is ready' check.
	exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	ENSURE(exts);	// else: called before OpenGL is ready for use
	have_12 = ogl_HaveVersion(1, 2);
	have_13 = ogl_HaveVersion(1, 3);
	have_14 = ogl_HaveVersion(1, 4);
	have_15 = ogl_HaveVersion(1, 5);
	have_20 = ogl_HaveVersion(2, 0);
	have_21 = ogl_HaveVersion(2, 1);
	have_30 = ogl_HaveVersion(3, 0);

	enableDummyFunctions();

	return true;
}


void ogl_SetVsyncEnabled(bool enabled)
{
	const int interval = enabled ? 1 : 0;
#if !CONFIG2_GLES && OS_WIN
	if (ogl_HaveExtension("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(interval);
#elif !CONFIG2_GLES && !OS_MACOSX && !OS_MAC
#if defined(SDL_VIDEO_DRIVER_X11)
	if (GLXVersion && ogl_HaveExtension("GLX_SGI_swap_control"))
		glXSwapIntervalSGI(interval);
#else
	UNUSED2(interval);
#endif
#else
	UNUSED2(interval);
#endif
}
