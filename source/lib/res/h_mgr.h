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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>		// type init routines get va_list of args

#include "../types.h"


// handle type (for 'type safety' - can't use a texture handle as a sound)
//
// rationale: we could use the destructor passed to h_alloc to identify
// the handle, but it's good to have a list of all types, and we avoid having
// to create empty destructors for handle types that wouldn't need them.
// finally, we save memory - this fits in a few bits, vs. needing a pointer.

// registering extension for each module is bad - some may use many
// (e.g. texture - many formats).
// handle manager shouldn't know about handle types

/*
enum H_Type
{
	H_Mem      = 1,
	H_ZArchive = 2,
	H_ZFile    = 3,
	H_VFile    = 4,
	H_VRead    = 5,

	H_Tex      = 6,
	H_Font     = 7,
	H_Sound    = 8,

	NUM_HANDLE_TYPES
};
*/

/*
///xxx advantage of manual vtbl:
no boilerplate init, h_alloc calls ctor directly, make sure it fits in the memory slot
vtbl contains sizeof resource data, and name!
but- has to handle variable params, a bit ugly
*/

// 'manual vtbl' type id
// handles have a type, to prevent using e.g. texture handles as a sound.
//
// alternatives:
// - enum of all handle types (smaller, have to pass all methods to h_alloc)
// - class (difficult to compare type, handle manager needs to know of all users)
//
// checked in h_alloc:
// - user_size must fit in what res.cpp is offering (currently 44 bytes)
// - name must not be 0
//
// init: user data is initially zeroed
// dtor: user data is zeroed automatically afterwards
// reload: if this resource type is opened by another resource's reload,
// our reload routine MUST check if already opened! This is relevant when
// a file is invalidated: if e.g. a sound object opens a file, the handle
// manager calls the reload routines for the 2 handles in unspecified order.
// ensuring the order would require a tag field that can't overflow -
// not really guaranteed with 32-bit handles. it'd also be more work
// to sort the handles by creation time, or account for several layers of
// dependencies.
struct H_VTbl
{
	void(*init)(void* user, va_list);
	int(*reload)(void* user, const char* fn);
	void(*dtor)(void* user);
	size_t user_size;
	const char* name;
};

typedef H_VTbl* H_Type;

#define H_TYPE_DEFINE(t)\
static void t##_init(t*, va_list);\
static int t##_reload(t*, const char*);\
static void t##_dtor(t*);\
static H_VTbl V_##t = {\
	(void(*)(void*, va_list))t##_init,\
	(int(*)(void*, const char*))t##_reload,\
	(void(*)(void*))t##_dtor,\
	sizeof(t),\
	#t\
};\
static H_Type H_##t = &V_##t;

// <type>* <var> = H_USER_DATA(<h_var>, <type>)
#define H_USER_DATA(h, type) (type*)h_user_data(h, H_##type);

#define H_DEREF(h, type, var)\
	type* const var = (type*)h_user_data(h, H_##type);\
	if(!var)\
		return ERR_INVALID_HANDLE;


// 0 = invalid handle value; < 0 is an error code.
// 64 bits, because we want tags to remain unique: tag overflow may
// let handle use errors slip through, or worse, cause spurious errors.
// with 32 bits, we'd need >= 12 for the index, leaving < 512K tags -
// not a lot.
typedef i64 Handle;


// all functions check the passed tag (part of the handle) and type against
// the internal values. if they differ, an error is returned.



// resource scope
// used together with flags (e.g. in mem), so no separate type
enum
{
	RES_TEMP   = 1,
	RES_LEVEL  = 2,
	RES_STATIC = 3
};

#define RES_SCOPE_MASK 3

// h_alloc flags
enum
{
	// the resource isn't backed by a file. the fn parameter is treated as the search key (uintptr_t)
	// currently only used by mem manager
	RES_KEY = 4
};



// allocate a new handle.
// if key is 0, or a (key, type) handle doesn't exist,
//   the first free entry is used.
// otherwise, a handle to the existing object is returned,
//   and HDATA.size != 0.
//// user_size is checked to make sure the user data fits in the handle data space.
// dtor is associated with type and called when the object is freed.
// handle data is initialized to 0; optionally, a pointer to it is returned.
extern Handle h_alloc(H_Type type, const char* fn, uint flags = 0, ...);
extern int h_free(Handle& h, H_Type type);

/*
// find and return a handle by key (typically filename hash)
// currently O(n).
extern Handle h_find(H_Type type, uintptr_t key);
*/

// return a pointer to handle data
extern void* h_user_data(Handle h, H_Type type);

extern const char* h_filename(Handle h);

// some resource types are heavyweight and reused between files (e.g. VRead).
// this reassigns dst's (of type <type>) associated file to that of src.
int h_reassign(const Handle dst, const H_Type dst_type, const Handle src);


extern int res_cur_scope;

#ifdef __cplusplus
}
#endif


#endif	// #ifndef __RES_H__
