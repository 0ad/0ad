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
//
// rationale: we could use the destructor passed to h_alloc to identify
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




// 0 = invalid handle value.
typedef u32 Handle;


// destructor, registered by h_alloc for a given handle type.
// receives the user data associated with the handle.
typedef void(*H_DTOR)(void*);


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
extern Handle h_alloc(u32 key, uint type,/* size_t user_size,*/ H_DTOR dtor = 0, void** puser = 0);
extern int h_free(Handle& h, uint type);

// find and return a handle by type and key (typically filename hash)
// currently O(n).
extern Handle h_find(u32 key, uint type, void** puser = 0);

// return a pointer to handle data
extern void* h_user_data(Handle h, uint type);


#endif	// #ifndef __RES_H__
