// OpenGL helpers
//
// Copyright (c) 2002-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"


#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "lib.h"
#include "sdl.h"
#include "ogl.h"
#include "detect.h"
#include "debug.h"
#include "lib/res/h_mgr.h"
#include "lib/res/graphics/tex.h"

#if MSC_VERSION
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
	// glu32 is required - it is apparently pulled in by opengl32,
	// even though depends.exe marks it as demand-loaded.
#endif


//----------------------------------------------------------------------------
// extensions
//----------------------------------------------------------------------------

// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (CALL_CONV *name) params;
#define FUNC2(ret, nameARB, nameCore, version, params) ret (CALL_CONV *nameARB) params;
#include "glext_funcs.h"
#undef FUNC2
#undef FUNC
}


static const char* exts = NULL;
static bool have_14, have_13, have_12;


// return a C string of unspecified length containing a space-separated
// list of all extensions the OpenGL implementation advertises.
// (useful for crash logs).
const char* oglExtList()
{
	debug_assert(exts && "call oglInit before using this function");
	return exts;
}


// paranoia: newer drivers may forget to advertise an extension
// indicating support for something that has been folded into the core.
// we therefore check for all extensions known to be offered by the
// GL implementation present on the user's system; oglHaveExtension will
// take this into account.
// the app can therefore just ask for extensions and not worry about this.
static bool isImplementedInCore(const char* ext)
{
#define MATCH(known_ext)\
	if(!strcmp(ext, #known_ext))\
		return true;

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
bool oglHaveExtension(const char* ext)
{
	debug_assert(exts && "call oglInit before using this function");

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
bool oglHaveVersion(const char* desired_version)
{
	int desired_major, desired_minor;
	if(sscanf(desired_version, "%d.%d", &desired_major, &desired_minor) != 2)
	{
		debug_warn("oglHaveVersion: invalid version string");
		return false;
	}

	int major, minor;
	const char* version = (const char*)glGetString(GL_VERSION);
	if(!version || sscanf(version, "%d.%d", &major, &minor) != 2)
	{
		debug_warn("oglHaveVersion: GL_VERSION invalid");
		return false;
	}

	return (major > desired_major) ||
	       (major == desired_major && minor >= desired_minor);
}


// check if all given extension strings (passed as const char* parameters,
// terminated by a 0 pointer) are supported by the OpenGL implementation,
// as determined by oglHaveExtension.
// returns 0 if all are present; otherwise, the first extension in the
// list that's not supported (useful for reporting errors).
//
// note: dummy parameter is necessary to access parameter va_list.
//
//
// rationale: this interface is more convenient than individual
// oglHaveExtension calls and allows reporting which extension is missing.
//
// one disadvantage is that there is no way to indicate that either one
// of 2 extensions would be acceptable, e.g. (ARB|EXT)_texture_env_dot3.
// this is isn't so bad, since they wouldn't be named differently
// if there weren't non-trivial changes between them. for that reason,
// we refrain from equivalence checks (which would boil down to
// string-matching known extensions to their equivalents).
const char* oglHaveExtensions(int dummy, ...)
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
		if(!oglHaveExtension(ext))
			break;
	}
	va_end(args);

	return ext;
}


static void importExtensionFunctions()
{
#define FUNC(ret, name, params) *(void**)&name = SDL_GL_GetProcAddress(#name);
#define FUNC2(ret, nameARB, nameCore, version, params) \
	nameARB = NULL; \
	if(oglHaveVersion(version)) \
		*(void**)&nameARB = SDL_GL_GetProcAddress(#nameCore); \
	if(!nameARB) /* use the ARB name if the driver lied about what version it supports */ \
		*(void**)&nameARB = SDL_GL_GetProcAddress(#nameARB);
#include "glext_funcs.h"
#undef FUNC2
#undef FUNC
		// It should be safe to load the ARB function pointers even if the
		// extension isn't advertised, since we won't actually use them without
		// checking for the extension.
}


//----------------------------------------------------------------------------

static void dump_gl_error(GLenum err)
{
	debug_printf("GL error: ");
#define E(e) case e: debug_printf("%s\n", #e); break;
	switch (err)
	{
	E(GL_INVALID_ENUM)
	E(GL_INVALID_VALUE)
	E(GL_INVALID_OPERATION)
	E(GL_STACK_OVERFLOW)
	E(GL_STACK_UNDERFLOW)
	E(GL_OUT_OF_MEMORY)
	default:;
	}
#undef E
}

#ifndef oglCheck
	// don't include this function if it has been defined (in ogl.h) as a no-op
void oglCheck()
{
	// glGetError may return multiple errors, so we poll it in a loop.
	// the debug_warn should only happen once (if this is set), though.
	bool error_enountered = false;

	for(;;)
	{
		GLenum err = glGetError();
		if(err == GL_NO_ERROR)
			break;

		error_enountered = true;
		dump_gl_error(err);
	}

	if(error_enountered)
		debug_warn("oglCheck reports error(s)");
}
#endif


// ignore and reset the specified error (as returned by glGetError).
// any other errors that have occurred are reported as oglCheck would.
//
// this is useful for suppressing annoying error messages, e.g.
// "invalid enum" for GL_CLAMP_TO_EDGE even though we've already
// warned the user that their OpenGL implementation is too old.
void oglSquelchError(GLenum err_to_ignore)
{
	// glGetError may return multiple errors, so we poll it in a loop.
	// the debug_warn should only happen once (if this is set), though.
	bool error_enountered = false;

	for(;;)
	{
		GLenum err = glGetError();
		if(err == GL_NO_ERROR)
			break;

		if(err == err_to_ignore)
			continue;

		error_enountered = true;
		dump_gl_error(err);
	}

	if(error_enountered)
		debug_warn("oglSquelchError reports other error(s)");
}


//----------------------------------------------------------------------------
// feature and limit detect
//----------------------------------------------------------------------------

int ogl_max_tex_size = -1;				// [pixels]
int ogl_max_tex_units = -1;				// limit on GL_TEXTUREn
int ogl_max_VAR_elements = -1;			// GF2: 64K; GF3: 1M
int ogl_tex_compression_supported = -1;	// S3TC / DXT{1,3,5}


// gfx_card and gfx_drv_ver are unchanged on failure.
int ogl_get_gfx_info()
{
	const char* vendor   = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version  = (const char*)glGetString(GL_VERSION);

	// can fail if OpenGL not yet initialized,
	// or if called between glBegin and glEnd.
	if(!vendor || !renderer || !version)
		return -1;

	strcpy_s(gfx_card, sizeof(gfx_card), vendor);

	// reduce string to "ATI" or "NVIDIA"
	if(!strcmp(gfx_card, "ATI Technologies Inc."))
		gfx_card[3] = 0;
	if(!strcmp(gfx_card, "NVIDIA Corporation"))
		gfx_card[6] = 0;

	strcat_s(gfx_card, sizeof(gfx_card), renderer);
		// don't bother cutting off the crap at the end.
		// too risky, and too many different strings.

	snprintf(gfx_drv_ver, sizeof(gfx_drv_ver), "OpenGL %s", version);
		// add "OpenGL" to differentiate this from the real driver version
		// (returned by platform-specific detect routines).

	return 0;
}


static void CALL_CONV emulate_glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);

static void detectFeatures()
{
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &ogl_max_tex_size);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &ogl_max_tex_units);
	// make sure value remains -1 if not supported
	if(oglHaveExtension("GL_NV_vertex_array_range"))
		glGetIntegerv(GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV, &ogl_max_VAR_elements);

	ogl_tex_compression_supported = oglHaveExtensions(0, "GL_ARB_texture_compression", "GL_EXT_texture_compression_s3tc", 0) == 0;
	// TODO: GL_S3_s3tc? It uses different enumerants (GL_RGB_S3TC etc), so the
	// texture loading code would need to be changed; and it is not clear whether
	// it supports the full range of DXT1/3/5. (There seems to be no specification;
	// and many header files don't have GL_RGBA_DXT5_S3TC, suggesting that the
	// drivers don't all support that.)

	if(!ogl_tex_compression_supported)
	{
		// If there's no hardware support for compressed textures, do the
		// decompression in software (but first let the user know it's probably not
		// going to be very fast).
		DISPLAY_ERROR(L"Performance warning: your graphics card does not support compressed textures. The game will try to continue anyway, but may be slower than expected. Please try updating your graphics drivers; if that doesn't help, please try upgrading your hardware.");
		// TODO: i18n
		glCompressedTexImage2DARB = emulate_glCompressedTexImage2D;

		// Leave ogl_tex_compression_supported == 0, so that it indicates the presence
		// of hardware-supported texture compression.
	}
}


// call after each video mode change, since thereafter extension functions
// may have changed [address].
void oglInit()
{
	// cache extension list and versions for oglHave*.
	// note: this is less about performance (since the above are not
	// time-critical) than centralizing the 'OpenGL is ready' check.
	exts = (const char*)glGetString(GL_EXTENSIONS);
	if(!exts)
	{
		debug_warn("oglInit called before OpenGL is ready for use");
	}
	have_12 = oglHaveVersion("1.2");
	have_13 = oglHaveVersion("1.3");
	have_14 = oglHaveVersion("1.4");

	importExtensionFunctions();

	detectFeatures();
}


uint ogl_dxt_from_fmt(GLenum fmt)
{
	switch(fmt)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return 1;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return DXT1A;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return 3;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return 5;
	default:
		return 0;
	}
}


// Software emulation of compressed-texture support, for really old
// cards/drivers that can't do it (but which do support everything else
// we strictly require). They probably don't have enough VRAM for all the
// textures, and are slow anyway, so it's not going to be a pleasant way
// of playing; but at least it's better than nothing.
static void CALL_CONV emulate_glCompressedTexImage2D(
	GLenum target_, GLint level_, GLenum int_fmt,
	GLsizei w, GLsizei h, GLint border,
	GLsizei data_size, const GLvoid* data)
{
	debug_assert(border == 0);	// not supported by glCompressedTexImage2D

	// S3TC format info
	const uint dxt = ogl_dxt_from_fmt(int_fmt);
	debug_assert(dxt != 0 && "invalid internal format");
	const uint s3tc_bpp = (dxt == 3 || dxt == 5)? 8 : 4;
	const size_t s3tc_size = round_up(w,4)*round_up(h,4)*s3tc_bpp/8;
	debug_assert(s3tc_size == (size_t)data_size);

	// decompress DDS to RGB[A]
	Tex t;
	const uint flags = dxt;
	(void)tex_wrap((uint)w, (uint)h, s3tc_bpp, flags, (void*)data, &t);
	// .. hack: prevent <data> from being freed when tex_transform
	//    replaces previous hm with the new transformed buffer.
	//    (important because mipmapped images share the same mem alloc and
	//    there's no way we know when to free that)
	h_add_ref(t.hm);
	(void)tex_transform(&t, TEX_DXT);

	// uncompressed RGB[A] format info
	u8* const uc_data = tex_get_data(&t);
	GLenum uc_fmt    = GL_RGB;
	GLint uc_int_fmt = GL_RGB8;
	if(dxt != 1)
	{
		uc_fmt     = GL_RGBA;
		uc_int_fmt = GL_RGBA8;
	}

	glTexImage2D(target_, level_, uc_int_fmt, w, h, 0, uc_fmt, GL_UNSIGNED_BYTE, uc_data);

	(void)tex_free(&t);
}