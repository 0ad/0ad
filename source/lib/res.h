// handle based caching resource manager
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

#ifndef __RES_H__
#define __RES_H__

#include "types.h"


// handle type (for 'type safety' - can't use a texture handle as a sound)
enum
{
	RES_TEX = 1,
	RES_FONT,
	RES_SOUND,
	RES_ZFILE,
	RES_ZARCHIVE,
	RES_VFILE,

	RES_MMAP,
	RES_MEM
};


typedef unsigned long Handle;

const int HDATA_INTERNAL_SIZE = 24;


struct HDATA
{
	void* p;
	size_t size;
	u8 internal[HDATA_INTERNAL_SIZE];
};

typedef void(*DTOR)(HDATA*);

extern Handle h_alloc(u32 key, uint type, DTOR dtor = 0, HDATA** phd = 0);
extern int h_free(Handle h, uint type);

// find and return a handle by type and associated key (typically filename hash)
// currently O(n).
extern Handle h_find(u32 key, uint type, HDATA** phd = 0);

// same as above, but search for a pointer the handle references
extern Handle h_find(const void* p, uint type, HDATA** phd = 0);

// get a handle's associated data.
// returns 0 if the handle is invalid, or <type> doesn't match
extern HDATA* h_data(Handle h, uint type);

extern Handle res_load(const char* fn, uint type, DTOR dtor, void*& p, size_t& size, HDATA*& hd);


#endif	// #ifndef __RES_H__
