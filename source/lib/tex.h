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
#include "res.h"


struct TEX
{
	u32 width  : 16;
	u32 height : 16;
	u32 fmt    : 16;
	u32 bpp    : 8;		// 0 if S3TC
	u32 s3tc_img_size;
	const u8* ptr;
	uint id;
};


// load and return a handle to the texture given in <fn>.
// supports RAW, BMP, JP2, PNG, TGA, DDS
// optionally returns a copy of information about the texture.
extern Handle tex_load(const char* fn, TEX* tex_info = 0);

extern int tex_bind(Handle h);


extern int tex_filter;	// GL values; default: GL_LINEAR
extern uint tex_bpp;	// 16 or 32; default: 32

// upload the specified texture to OpenGL. Texture filter and internal format
// may be specified to override the global defaults.
extern int tex_upload(Handle h, int filter_override = 0, int internal_fmt_override = 0);

#endif	// __TEX_H__
