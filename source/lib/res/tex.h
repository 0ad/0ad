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

#include "h_mgr.h"



// TexInfo.flags
enum
{
	TEX_DXT   = 0x07,	// mask; value = {1,3,5}
	TEX_BGR   = 8,
	TEX_ALPHA = 16,
	TEX_GRAY  = 32,
};

// minimize size - stored in ogl tex resource control block
struct TexInfo
{
	Handle hm;			// H_MEM handle to loaded file
	size_t ofs;			// offset to image data in file
	u32 w : 16;
	u32 h : 16;
	u32 bpp : 16;
	u32 flags : 16;
};

extern int tex_load(const char* fn, TexInfo* ti);
extern int tex_free(TexInfo* ti);


#endif	// __TEX_H__
