/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/config2.h" // CONFIG2_GLES
#include "lib/sysdep/os.h" // OS_WIN


#if CONFIG2_GLES
# include "external_libraries/opengles2_wrapper.h"
#else
# include <glad/gl.h>
#endif

/**
 * initialization: import extension function pointers and do feature detect.
 * call before using any other function.
 * fails if OpenGL not ready for use.
 **/
#if OS_WIN
extern bool ogl_Init(void* (load)(const char*), void* hdc);
#elif !OS_MACOSX && !OS_MAC && !CONFIG2_GLES
extern bool ogl_Init(void* (load)(const char*), void* display);
#else
extern bool ogl_Init(void* (load)(const char*));
#endif

/**
 * Change vsync state.
 **/
extern void ogl_SetVsyncEnabled(bool enabled);

//-----------------------------------------------------------------------------
// extensions

/**
 * Check whether the given OpenGL extension is supported.
 * NOTE: this does not check whether the extensions is *loaded*.
 * for that, check whether GLAD_<extension name> is not null.
 *
 * @param ext extension string; exact case.
 * @return bool.
 **/
extern bool ogl_HaveExtension(const char* ext);

/**
 * make sure the OpenGL implementation version matches or is newer than
 * the given version.
 */
extern bool ogl_HaveVersion(int major, int minor);
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

#endif	// INCLUDED_OGL
