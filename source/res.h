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


enum
{
	RES_MEM   = 1,

	RES_BIN   = 2,
	RES_TEX   = 3,
	RES_FONT  = 4,
	RES_ZIP   = 5,
	RES_VFILE = 6,

	NUM_RES_TYPES
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

extern Handle h_alloc(u32 key, uint type, DTOR dtor, HDATA*& hd);
extern int h_free(Handle h, uint type = 0);
extern Handle h_find(u32 key, uint type, HDATA*& hd);
extern HDATA* h_data(Handle h, uint type);

extern Handle res_load(const char* fn, uint type, DTOR dtor, void*& p, size_t& size, HDATA*& hd);


#endif	// #ifndef __RES_H__
