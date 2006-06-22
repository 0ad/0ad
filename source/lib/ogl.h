/**
 * =========================================================================
 * File        : ogl.h
 * Project     : 0 A.D.
 * Description : OpenGL helper functions.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2002-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __OGL_H__
#define __OGL_H__

#ifdef __cplusplus
extern "C" {
#endif

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


// Enums from EXT_framebuffer_object
#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_RENDERBUFFER_RED_SIZE_EXT           0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT         0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT          0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT         0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT         0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT       0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_BINDING_EXT             0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT            0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT           0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT           0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT   0x0506
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

// set sysdep/gfx.h gfx_card and gfx_drv_ver. called by gfx_detect.
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
