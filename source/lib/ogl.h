/**
 * =========================================================================
 * File        : ogl.h
 * Project     : 0 A.D.
 * Description : OpenGL helper functions.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_OGL
#define INCLUDED_OGL

#if OS_WIN
// wgl.h is a private header and should only be included from here.
// if this isn't defined, it'll complain.
#define WGL_HEADER_NEEDED
#include "lib/sysdep/win/wgl.h"
#endif


//
// bring in the platform's OpenGL headers (with fixes, if necessary)
//

#if OS_MACOSX || OS_MAC
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

// if gl.h provides real prototypes for 1.2 / 1.3 functions,
// exclude the corresponding function pointers in glext_funcs.h
#ifdef GL_VERSION_1_2
#define REAL_GL_1_2
#endif
#ifdef GL_VERSION_1_3
#define REAL_GL_1_3
#endif

// this must come after GL/gl.h include, so we can't combine the
// including GL/glext.h.
#undef GL_GLEXT_PROTOTYPES

#if OS_MACOSX || OS_MAC
# include <OpenGL/glext.h>
#else
# include <GL/glext.h>
# if OS_WIN
#  include <GL/wglext.h>
#  define GL_TEXTURE_IMAGE_SIZE_ARB 0x86A0
# endif
#endif


/**
 * initialization: import extension function pointers and do feature detect.
 * call before using any other function, and after each video mode change.
 * fails if OpenGL not ready for use.
 **/
extern void ogl_Init(void);


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
extern const char* ogl_HaveExtensions(int dummy, ...);

/**
 * get a list of all supported extensions.
 *
 * useful for crash logs / system information.
 *
 * @return read-only C string of unspecified length containing all
 * advertised extension names, separated by space.
 **/
extern const char* ogl_ExtensionString(void);

// declare extension function pointers
#if OS_WIN
# define GL_CALL_CONV __stdcall
#else
# define GL_CALL_CONV
#endif
#define FUNC(ret, name, params) EXTERN_C ret (GL_CALL_CONV *p##name) params;
#define FUNC2(ret, nameARB, nameCore, version, params) EXTERN_C ret (GL_CALL_CONV *p##nameARB) params;
#include "glext_funcs.h"
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
extern void ogl_WarnIfError(void);
#ifdef NDEBUG
# define ogl_WarnIfError()
#endif

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
 **/
extern void ogl_SquelchError(GLenum err_to_ignore);


//-----------------------------------------------------------------------------
// implementation limits / feature detect

extern GLint ogl_max_tex_size;				/// [pixels]
extern GLint ogl_max_tex_units;				/// limit on GL_TEXTUREn

/**
 * set sysdep/gfx.h gfx_card and gfx_drv_ver. called by gfx_detect.
 * 
 * fails if OpenGL not ready for use.
 * gfx_card and gfx_drv_ver are unchanged on failure.
 *
 * @return LibError
 **/
extern LibError ogl_get_gfx_info(void);

#endif	// #ifndef INCLUDED_OGL
