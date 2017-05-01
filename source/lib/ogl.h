/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_OGL
#define INCLUDED_OGL

#include "lib/external_libraries/opengl.h"


/**
 * initialization: import extension function pointers and do feature detect.
 * call before using any other function, and after each video mode change.
 * fails if OpenGL not ready for use.
 **/
extern void ogl_Init();


//-----------------------------------------------------------------------------
// extensions

/**
 * check if an extension is supported by the OpenGL implementation.
 *
 * takes subsequently added core support for some extensions into account
 * (in case drivers forget to advertise extensions).
 *
 * @param ext extension string; exact case.
 * @return bool.
 **/
extern bool ogl_HaveExtension(const char* ext);

/**
 * make sure the OpenGL implementation version matches or is newer than
 * the given version.
 *
 * @param version version string; format: ("%d.%d", major, minor).
 * example: "1.2".
 **/
extern bool ogl_HaveVersion(const char* version);

/**
 * check if a list of extensions are all supported (as determined by
 * ogl_HaveExtension).
 *
 * @param dummy value ignored; varargs requires a placeholder.
 * follow it by a list of const char* extension string parameters,
 * terminated by a 0 pointer.
 * @return 0 if all are present; otherwise, the first extension in the
 * list that's not supported (useful for reporting errors).
 **/
extern const char* ogl_HaveExtensions(int dummy, ...) SENTINEL_ARG;

/**
 * get a list of all supported extensions.
 *
 * useful for crash logs / system information.
 *
 * @return read-only C string of unspecified length containing all
 * advertised extension names, separated by space.
 **/
extern const char* ogl_ExtensionString();

// The game wants to use some extension constants that aren't provided by
// glext.h on some old systems.
// Manually define all the necessary ones that are missing from
// GL_GLEXT_VERSION 39 (Mesa 7.0) since that's probably an old enough baseline:
#ifndef GL_VERSION_3_0
# define GL_MIN_PROGRAM_TEXEL_OFFSET 0x8904
# define GL_MAX_PROGRAM_TEXEL_OFFSET 0x8905
#endif
#ifndef GL_EXT_transform_feedback
# define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT 0x8C8A
# define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT 0x8C8B
# define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT 0x8C80
#endif
#ifndef GL_ARB_geometry_shader4
# define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB 0x8C29
# define GL_MAX_GEOMETRY_VARYING_COMPONENTS_ARB 0x8DDD
# define GL_MAX_VERTEX_VARYING_COMPONENTS_ARB 0x8DDE
# define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB 0x8DDF
# define GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB 0x8DE0
# define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB 0x8DE1
#endif
#ifndef GL_ARB_timer_query
# define GL_TIME_ELAPSED 0x88BF
# define GL_TIMESTAMP 0x8E28
#endif
#ifndef GL_ARB_framebuffer_object
# define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif
// Also need some more for OS X 10.5:
#ifndef GL_EXT_texture_array
# define GL_MAX_ARRAY_TEXTURE_LAYERS_EXT 0x88FF
#endif
// Also need some types not in old glext.h:
#ifndef GL_ARB_sync
 typedef int64_t GLint64;
 typedef uint64_t GLuint64;
#endif

// declare extension function pointers
#if OS_WIN
# define GL_CALL_CONV __stdcall
#else
# define GL_CALL_CONV
#endif
#define FUNC(ret, name, params) EXTERN_C ret (GL_CALL_CONV *p##name) params;
#define FUNC2(ret, nameARB, nameCore, version, params) EXTERN_C ret (GL_CALL_CONV *p##nameARB) params;
#define FUNC3(ret, nameARB, nameCore, version, params) EXTERN_C ret (GL_CALL_CONV *p##nameCore) params;
#include "lib/external_libraries/glext_funcs.h"
#undef FUNC3
#undef FUNC2
#undef FUNC
// leave GL_CALL_CONV defined for ogl.cpp


//-----------------------------------------------------------------------------
// errors

/**
 * raise a warning (break into the debugger) if an OpenGL error is pending.
 * resets the OpenGL error state afterwards.
 *
 * when an error is reported, insert calls to this in a binary-search scheme
 * to quickly narrow down the actual error location.
 *
 * reports a bogus invalid_operation error if called before OpenGL is
 * initialized, so don't!
 *
 * disabled in release mode for efficiency and to avoid annoying errors.
 **/
extern void ogl_WarnIfErrorLoc(const char *file, int line);
#ifdef NDEBUG
# define ogl_WarnIfError()
#else
# define ogl_WarnIfError() ogl_WarnIfErrorLoc(__FILE__, __LINE__)
#endif

/**
* get a name of the error.
*
* useful for debug.
*
* @return read-only C string of unspecified length containing
* the error's name.
**/
extern const char* ogl_GetErrorName(GLenum err);

/**
 * ignore and reset the specified OpenGL error.
 *
 * this is useful for suppressing annoying error messages, e.g.
 * "invalid enum" for GL_CLAMP_TO_EDGE even though we've already
 * warned the user that their OpenGL implementation is too old.
 *
 * call after the fact, i.e. the error has been raised. if another or
 * different error is pending, those are reported immediately.
 *
 * @param err_to_ignore: one of the glGetError enums.
 * @return true if the requested error was seen and ignored
 **/
extern bool ogl_SquelchError(GLenum err_to_ignore);


//-----------------------------------------------------------------------------
// implementation limits / feature detect

extern GLint ogl_max_tex_size;				/// [pixels]
extern GLint ogl_max_tex_units;				/// limit on GL_TEXTUREn

#endif	// #ifndef INCLUDED_OGL
