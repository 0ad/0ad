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

// the following are internal to the resource manager,
// but are required for the HDATA definition
// (which is passed around to modules that create handles).
// we don't want to waste memory or fragment the handle data
// by splitting into internal/external


// handle type (for 'type safety' - can't use a texture handle as a sound)
//
// rationale: we could use the destructor passed to res_load to identify
// the handle, but it's good to have a list of all types, and we avoid having
// to create empty destructors for handle types that wouldn't need them.
// finally, we save memory - this fits in a few bits, vs. needing a pointer.
enum HType
{
	H_TEX      = 1,
	H_FONT     = 2,
	H_SOUND    = 3,
	H_ZFILE    = 4,
	H_ZARCHIVE = 5,
	H_VFILE    = 6,
	H_MEM      = 7,

	NUM_HANDLE_TYPES
};

// handle's pointer type
enum PType
{
	PT_NONE,
	PT_MEM,			// allocated by mem_alloc
	PT_MAP			// mapped by mmap
};

// handle (32 bits)
// .. make sure this is the same handle we opened
const uint HTAG_BITS = 16;
// .. index into array => = log2(max open handles)
const uint HIDX_BITS = 12;

// <= 32-TAG_BITS bits
const uint HTYPE_BITS = 4;
const uint PTYPE_BITS = 4;
const uint HREF_BITS  = 8;

const int HDATA_USER_SIZE = 16;

// 32 bytes
struct HDATA
{
	u32 key;
	u32 tag   : HTAG_BITS;
	u32 type  : HTYPE_BITS;		// handle's type (e.g. texture, sound)
	u32 ptype : PTYPE_BITS;		// what does p point to?
	u32 refs  : HREF_BITS;

	void* p;
	size_t size;

	u8 user[HDATA_USER_SIZE];
};


// 0 = invalid handle value.
typedef u32 Handle;


// destructor, registered by h_alloc for a given handle type.
// receives the user data associated with the handle.
typedef void(*DTOR)(void*);


// all functions check the passed tag (part of the handle) and type against
// the internal values. if they differ, an error is returned.


// allocate a new handle.
// if key is 0, or a (key, type) handle doesn't exist,
//   the first free entry is used.
// otherwise, a handle to the existing object is returned,
//   and HDATA.size != 0.
//// user_size is checked to make sure the user data fits in the handle data space.
// dtor is associated with type and called when the object is freed.
// handle data is initialized to 0; optionally, a pointer to it is returned.
extern Handle h_alloc(u32 key, uint type,/* size_t user_size,*/ DTOR dtor = 0, HDATA** puser = 0);
extern int h_free(Handle h, uint type);

// find and return a handle by type and key (typically filename hash)
// currently O(n).
extern Handle h_find(u32 key, uint type, HDATA** puser = 0);

// same as above, but search for the handle's associated pointer
extern Handle h_find(const void* p, uint type, HDATA** puser = 0);

// return a pointer to handle data
extern HDATA* h_data(Handle h, uint type);

extern Handle res_load(const char* fn, uint type, DTOR dtor, void*& p, size_t& size, HDATA*& user);


#endif	// #ifndef __RES_H__
