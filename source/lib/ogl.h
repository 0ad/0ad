#ifndef __OGL_H__
#define __OGL_H__

#ifdef __cplusplus
extern "C" {
#endif

#if OS_WIN
#include "sysdep/win/wgl.h"
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


//
// extensions
//

// check if the extension <ext> is supported by the OpenGL implementation.
// takes subsequently added core support for some extensions into account.
extern bool oglHaveExtension(const char* ext);

// check if the OpenGL implementation is at least at <version>.
// (format: "%d.%d" major minor)
extern bool oglHaveVersion(const char* version);

// check if all given extension strings (passed as const char* parameters,
// terminated by a 0 pointer) are supported by the OpenGL implementation,
// as determined by oglHaveExtension.
// returns 0 if all are present; otherwise, the first extension in the
// list that's not supported (useful for reporting errors).
//
// note: dummy parameter is necessary to access parameter va_list.
//
// rationale: see source.
extern const char* oglHaveExtensions(int dummy, ...);

// return a C string of unspecified length containing a space-separated
// list of all extensions the OpenGL implementation advertises.
// (useful for crash logs).
extern const char* oglExtList(void);


#if OS_WIN
# define CALL_CONV __stdcall
#else
# define CALL_CONV
#endif

#define FUNC(ret, name, params) extern ret (CALL_CONV *p##name) params;
#define FUNC2(ret, nameARB, nameCore, version, params) extern ret (CALL_CONV *p##nameARB) params;
#include "glext_funcs.h"
#undef FUNC2
#undef FUNC

// leave CALL_CONV defined for ogl.cpp


//
// implementation limits / feature detect
//

extern int ogl_max_tex_size;				// [pixels]
extern int ogl_max_tex_units;				// limit on GL_TEXTUREn

// set detect.cpp gfx_card[] and gfx_drv_ver[].
// (called by detect.cpp get_gfx_info()).
// 
// fails if OpenGL not ready for use.
// gfx_card and gfx_drv_ver are unchanged on failure.
extern LibError ogl_get_gfx_info(void);


//
// misc
//

// in non-release builds, enable oglCheck, which breaks into the debugger
// if an OpenGL error was raised since the last call.
// add these calls everywhere to close in on the error cause.
//
// reports a bogus invalid_operation error if called before OpenGL is
// initialized, so don't!
#ifndef NDEBUG
extern void oglCheck(void);
#else
# define oglCheck()
#endif

// ignore and reset the specified error (as returned by glGetError).
// any other errors that have occurred are reported as oglCheck would.
//
// this is useful for suppressing annoying error messages, e.g.
// "invalid enum" for GL_CLAMP_TO_EDGE even though we've already
// warned the user that their OpenGL implementation is too old.
extern void oglSquelchError(GLenum err_to_ignore);


// call before using any of the above, and after each video mode change.
//
// fails if OpenGL not ready for use.
extern void oglInit(void);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __OGL_H__
