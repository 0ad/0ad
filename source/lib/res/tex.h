// OpenGL texturing
//
// Copyright (c) 2003 Jan Wassenberg
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

#ifndef __TEX_H__
#define __TEX_H__

#include "types.h"
#include "h_mgr.h"
#include "misc.h"

// load and return a handle to the texture given in <fn>.
// supports RAW, BMP, JP2, PNG, TGA, DDS
extern Handle tex_load(const char* const fn, int scope = 0);

extern int tex_bind(Handle ht);

extern int tex_info(Handle ht, int* w, int* h, void** p);

extern int tex_filter;	// GL values; default: GL_LINEAR
extern uint tex_bpp;	// 16 or 32; default: 32

// upload the specified texture to OpenGL. Texture filter and internal format
// may be specified to override the global defaults.
extern int tex_upload(Handle ht, int filter_override = 0, int internal_fmt_override = 0);

extern int tex_free(Handle& ht);

#endif	// __TEX_H__
