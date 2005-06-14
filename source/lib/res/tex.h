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

#include "handle.h"



enum TexInfoFlags
{
	TEX_DXT   = 0x07,	// mask; value = {1,3,5}
	TEX_BGR   = 0x08,
	TEX_ALPHA = 0x10,
	TEX_GRAY  = 0x20,

	// orientation - never returned by tex_load, since it automatically
	// flips to match global orientation. these are passed to tex_write
	// to indicate the image orientation, or to tex_set_global_orientation.
	TEX_BOTTOM_UP = 0x40,
	TEX_TOP_DOWN  = 0x80,
	TEX_ORIENTATION = TEX_BOTTOM_UP|TEX_TOP_DOWN,	// mask

	// mipmaps - if this flag is set, the image contains data for all
	// mipmap levels down to 1x1, stored contiguously.
	TEX_MIPMAPS = 0x100
};

// minimize size - stored in ogl tex resource control block
struct TexInfo
{
	Handle hm;			// H_Mem handle to loaded file
	size_t ofs;			// offset to image data in file
	uint w : 16;
	uint h : 16;
	uint bpp : 16;		// average bits per pixel
	uint flags : 16;
};

extern int tex_load(const char* fn, TexInfo* ti);
extern int tex_load_mem(Handle hm, const char* fn, TexInfo* t);
extern int tex_free(TexInfo* ti);

extern int tex_write(const char* fn, uint w, uint h, uint bpp, uint flags, void* img);

extern int tex_is_known_fmt(void* p, size_t size_t);

// param: either TEX_BOTTOM_UP or TEX_TOP_DOWN
extern void tex_set_global_orientation(int orientation);

#endif	// __TEX_H__
