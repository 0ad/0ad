#ifndef OGL_TEX_H__
#define OGL_TEX_H__

#include "../handle.h"
#include "lib/ogl.h"
#include "lib/types.h"

// load and return a handle to the texture given in <fn>.
// supports RAW, BMP, JP2, PNG, TGA, DDS
extern Handle ogl_tex_load(const char* fn, int scope = 0);

extern int ogl_tex_bind(Handle ht, GLenum unit = 0);

extern GLint ogl_tex_filter;	// GL values; default: GL_LINEAR
extern uint ogl_tex_bpp;		// 16 or 32; default: 32

// upload the specified texture to OpenGL. Texture filter and internal format
// may be specified to override the global defaults.
// side effect: binds the texture to the currently active unit.
extern int ogl_tex_upload(Handle ht, GLint filter_override = 0, GLint internal_fmt_override = 0, GLenum format_override = 0);

extern int ogl_tex_free(Handle& ht);

extern int ogl_tex_get_size(Handle ht, int* w, int* h, int* bpp);
extern int ogl_tex_get_format(Handle ht, int* flags, GLenum* fmt);
extern int ogl_tex_get_data(Handle ht, void** p);

#endif	// #ifndef OGL_TEX_H__
