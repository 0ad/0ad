//
// Copyright (c) 2002 Jan Wassenberg
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


struct TEX
{
	uint width;
	uint height;
	uint fmt;
	const u8* ptr;
	uint id;
	uint img_size;	// currently only used for S3TC
};


extern u32 tex_load(const char* fn, TEX* ti = 0);

extern int tex_bind(u32 h);


const int BILINEAR = 2;
const int TRINILEAR = 6;

extern uint tex_filter_quality; // 1..6; default: BILINEAR
extern uint tex_bpp;			// 16 or 32; default: 32

extern int tex_upload(u32 h, uint filter_quality = 0, uint internal_fmt = 0);

#endif	// __TEX_H__
