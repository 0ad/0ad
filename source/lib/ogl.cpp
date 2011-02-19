/* Copyright (c) 2010 Wildfire Games
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

/*
 * OpenGL helper functions.
 */

#include "precompiled.h"
#include "lib/ogl.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "lib/external_libraries/sdl.h"
#include "lib/debug.h"
#include "lib/sysdep/gfx.h"
#include "lib/res/h_mgr.h"

#if MSC_VERSION
#pragma comment(lib, "opengl32.lib")
#endif


//----------------------------------------------------------------------------
// extensions
//----------------------------------------------------------------------------

// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (GL_CALL_CONV *p##name) params;
#define FUNC2(ret, nameARB, nameCore, version, params) ret (GL_CALL_CONV *p##nameARB) params;
#define FUNC3(ret, nameARB, nameCore, version, params) ret (GL_CALL_CONV *p##nameCore) params;
#include "lib/glext_funcs.h"
#undef FUNC3
#undef FUNC2
#undef FUNC
}


static const char* exts = NULL;
static bool have_20, have_14, have_13, have_12;


// return a C string of unspecified length containing a space-separated
// list of all extensions the OpenGL implementation advertises.
// (useful for crash logs).
const char* ogl_ExtensionString()
{
	debug_assert(exts && "call ogl_Init before using this function");
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

	if(have_20)
	{
		MATCH(GL_ARB_shader_objects);
		MATCH(GL_ARB_vertex_shader);
		MATCH(GL_ARB_fragment_shader);
		MATCH(GL_ARB_draw_buffers);
		MATCH(GL_ARB_texture_non_power_of_two);
		MATCH(GL_ARB_point_sprite);
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
		MATCH(GL_EXT_texture3d);
		MATCH(GL_EXT_bgra);
		MATCH(GL_EXT_packed_pixels);
		MATCH(GL_EXT_rescale_normal);
		MATCH(GL_EXT_separate_specular_color);
		MATCH(GL_SGIS_texture_edge_clamp);
		MATCH(GL_SGIS_texture_lod);
		MATCH(GL_EXT_draw_range_elements);
	}

#undef MATCH
	return false;
}


// check if the extension <ext> is supported by the OpenGL implementation.
// takes subsequently added core support for some extensions into account.
bool ogl_HaveExtension(const char* ext)
{
	debug_assert(exts && "call ogl_Init before using this function");

	if(isImplementedInCore(ext))
		return true;

	const char *p = exts, *end;

	// make sure ext is valid & doesn't contain spaces
	if(!ext || ext == '\0' || strchr(ext, ' '))
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


// check if the OpenGL implementation is at least at <version>.
// (format: "%d.%d" major minor)
bool ogl_HaveVersion(const char* desired_version)
{
	int desired_major, desired_minor;
	if(sscanf_s(desired_version, "%d.%d", &desired_major, &desired_minor) != 2)
	{
		debug_assert(0);	// invalid version string
		return false;
	}

	// guaranteed to be of the form major.minor[.release].
	const char* version = (const char*)glGetString(GL_VERSION);
	int major, minor;
	if(!version || sscanf_s(version, "%d.%d", &major, &minor) != 2)
	{
		debug_assert(0);	// GL_VERSION invalid
		return false;
	}

	// note: don't just compare characters - major and minor may be >= 10.
	return (major > desired_major) ||
	       (major == desired_major && minor >= desired_minor);
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

static void GL_CALL_CONV dummy_glDrawRangeElementsEXT(GLenum mode, GLuint, GLuint, GLsizei count, GLenum type, GLvoid* indices)
{
	glDrawElements(mode, count, type, indices);
}

static void GL_CALL_CONV dummy_glActiveTextureARB(int)
{
}

static void GL_CALL_CONV dummy_glClientActiveTextureARB(int)
{
}

static void GL_CALL_CONV dummy_glMultiTexCoord2fARB(int, float s, float t)
{
	glTexCoord2f(s, t);
}

static void importExtensionFunctions()
{
	// It should be safe to load the ARB function pointers even if the
	// extension isn't advertised, since we won't actually use them without
	// checking for the extension.
	// (TODO: this calls ogl_HaveVersion far more times than is necessary -
	// we should probably use the have_* variables instead)
#define FUNC(ret, name, params) p##name = (ret (GL_CALL_CONV*) params)SDL_GL_GetProcAddress(#name);
#define FUNC23(pname, ret, nameARB, nameCore, version, params) \
	pname = NULL; \
	if(ogl_HaveVersion(version)) \
		pname = (ret (GL_CALL_CONV*) params)SDL_GL_GetProcAddress(#nameCore); \
	if(!pname) /* use the ARB name if the driver lied about what version it supports */ \
		pname = (ret (GL_CALL_CONV*) params)SDL_GL_GetProcAddress(#nameARB);
#define FUNC2(ret, nameARB, nameCore, version, params) FUNC23(p##nameARB, ret, nameARB, nameCore, version, params)
#define FUNC3(ret, nameARB, nameCore, version, params) FUNC23(p##nameCore, ret, nameARB, nameCore, version, params)
#include "lib/glext_funcs.h"
#undef FUNC3
#undef FUNC2
#undef FUNC23
#undef FUNC

	// fall back to the dummy functions
	if(!pglDrawRangeElementsEXT)
		pglDrawRangeElementsEXT = &dummy_glDrawRangeElementsEXT;
	if(!pglActiveTextureARB)
		pglActiveTextureARB = &dummy_glActiveTextureARB;
	if(!pglClientActiveTextureARB)
		pglClientActiveTextureARB = &dummy_glClientActiveTextureARB;
	if(!pglMultiTexCoord2fARB)
		pglMultiTexCoord2fARB = &dummy_glMultiTexCoord2fARB;
}


//----------------------------------------------------------------------------

static void dump_gl_error(GLenum err)
{
	debug_printf(L"OGL| ");
#define E(e) case e: debug_printf(L"%ls\n", WIDEN(#e)); break;
	switch (err)
	{
	E(GL_INVALID_ENUM)
	E(GL_INVALID_VALUE)
	E(GL_INVALID_OPERATION)
	E(GL_STACK_OVERFLOW)
	E(GL_STACK_UNDERFLOW)
	E(GL_OUT_OF_MEMORY)
	E(GL_INVALID_FRAMEBUFFER_OPERATION_EXT)
	default: debug_printf(L"Unknown GL error: %04x\n", err); break;
	}
#undef E
}

#ifndef ogl_WarnIfError
	// don't include this function if it has been defined (in ogl.h) as a no-op
void ogl_WarnIfError()
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
		debug_printf(L"OpenGL error(s) occurred: %04x\n", (int)first_error);
}
#endif


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
		debug_printf(L"OpenGL error(s) occurred: %04x\n", (int)first_error);

	return error_ignored;
}


//----------------------------------------------------------------------------
// feature and limit detect
//----------------------------------------------------------------------------

GLint ogl_max_tex_size = -1;				// [pixels]
GLint ogl_max_tex_units = -1;				// limit on GL_TEXTUREn


// set sysdep/gfx.h gfx_card and gfx_drv_ver. called by gfx_detect.
//
// fails if OpenGL not ready for use.
// gfx_card and gfx_drv_ver are unchanged on failure.
LibError ogl_get_gfx_info()
{
	const char* vendor   = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version  = (const char*)glGetString(GL_VERSION);
	// can fail if OpenGL not yet initialized,
	// or if called between glBegin and glEnd.
	if(!vendor || !renderer || !version)
		WARN_RETURN(ERR::AGAIN);

	swprintf_s(gfx_card, ARRAY_SIZE(gfx_card), L"%hs %hs", vendor, renderer);

	// add "OpenGL" to differentiate this from the real driver version
	// (returned by platform-specific detect routines).
	swprintf_s(gfx_drv_ver, ARRAY_SIZE(gfx_drv_ver), L"OpenGL %hs", version);
	return INFO::OK;
}


// call after each video mode change, since thereafter extension functions
// may have changed [address].
void ogl_Init()
{
	// cache extension list and versions for oglHave*.
	// note: this is less about performance (since the above are not
	// time-critical) than centralizing the 'OpenGL is ready' check.
	exts = (const char*)glGetString(GL_EXTENSIONS);
	debug_assert(exts);	// else: called before OpenGL is ready for use
	have_12 = ogl_HaveVersion("1.2");
	have_13 = ogl_HaveVersion("1.3");
	have_14 = ogl_HaveVersion("1.4");
	have_20 = ogl_HaveVersion("2.0");

	importExtensionFunctions();

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &ogl_max_tex_size);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &ogl_max_tex_units);
}
