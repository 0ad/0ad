// handle manager
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

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>

#include "lib.h"
#include "h_mgr.h"
#include "mem.h"

// TODO: h_find - required for caching


//
// handle
//

// handles are an indirection layer between client code and resources.
// they allow an important check not possible with a direct pointer:
// guaranteeing the handle references a given resource /instance/.
//
// problem: code C1 allocates a resource, and receives a pointer p to its
// control block. C1 passes p on to C2, and later frees it.
// now other code allocates a resource, and happens to reuse the free slot
// pointed to by p (also possible if simply allocating from the heap).
// when C2 accesses p, the pointer is valid, but we cannot tell that
// it is referring to a resource that had already been freed. big trouble.
//
// solution: each allocation receives a unique tag (a global counter that
// is large enough to never overflow). Handles include this tag, as well
// as a reference (array index) to the control block, which isn't directly
// accessible. when dereferencing the handle, we check if the handle's tag
// matches the copy stored in the control block. this protects against stale
// handle reuse, double-free, and accidentally referencing other resources.
//
// type: each handle has an associated type. these must be checked to prevent
// using textures as sounds, for example. with the manual vtbl scheme,
// this type is actually a pointer to the resource object's vtbl, and is
// set up via H_TYPE_DEFINE. see header for rationale. this means that
// types are private to the module that declared the handle; knowledge
// of the type ensures the caller actually declared, and owns the resource.

// 0 = invalid handle value
// < 0 is an error code (we assume < 0 <==> MSB is set - 
//     true for 1s and 2s complement and sign-magnitude systems)

// fields:
// (shift value = # bits between LSB and field LSB.
//  may be larger than the field type - only shift Handle vars!)

// - tag (1-based) ensures the handle references a certain resource instance.
//   (field width determines maximum unambiguous resource allocs)
#define TAG_BITS 32
const uint TAG_SHIFT = 0;
const u32 TAG_MASK = 0xffffffff;	// safer than (1 << 32) - 1

// - index (0-based) points to control block in our array.
//   (field width determines maximum currently open handles)
#define IDX_BITS 16
const uint IDX_SHIFT = 32;
const i32 IDX_MASK = (1l << IDX_BITS) - 1;

cassert(IDX_BITS + TAG_BITS <= sizeof(Handle)*CHAR_BIT);


// return the handle's index field (always non-negative).
// no error checking!
static inline i32 h_idx(const Handle h)
{ return (i32)((h >> IDX_SHIFT) & IDX_MASK); }

// return the handle's tag field.
// no error checking!
static inline u32 h_tag(const Handle h)
{ return (u32)((h >> TAG_SHIFT) & TAG_MASK); }

// build a handle from index and tag
static inline Handle handle(const i32 idx, const i32 tag)
{
	assert(idx <= IDX_MASK && tag <= TAG_MASK && "handle: idx or tag too big");
	// somewhat clunky, but be careful with the shift:
	// *_SHIFT may be larger than its field's type.
	Handle _idx = idx & IDX_MASK; _idx <<= IDX_SHIFT;
	Handle _tag = tag & TAG_MASK; _tag <<= TAG_SHIFT;
	return _idx | _tag;
}


//
// internal per-resource-instance data
//

// determines maximum number of references to a resource.
// a handle's idx field isn't stored in its HDATA entry (not needed);
// to save space, this should take its place, i.e. it should fit in IDX_BITS.
static const uint REF_BITS  = 12;
static const u32 REF_MAX = 1ul << REF_BITS;

// chosen so that all current resource structs are covered,
// and so sizeof(HDATA) is a power of 2 (for more efficient array access
// and array page usage).
static const size_t HDATA_USER_SIZE = 48;

// 64 bytes
struct HDATA
{
	uintptr_t key;
	u32 tag  : TAG_BITS;
	u32 refs : REF_BITS;
	H_Type type;

	const char* fn;

	u8 user[HDATA_USER_SIZE];
};


// max data array entries. compared to last_in_use => signed.
static const i32 hdata_cap = 1ul << IDX_BITS;

// allocate entries as needed so as not to waste memory
// (hdata_cap may be large). deque-style array of pages
// to balance locality, fragmentation, and waste.
static const size_t PAGE_SIZE = 4096;
static const uint hdata_per_page = PAGE_SIZE / sizeof(HDATA);
static const uint num_pages = hdata_cap / hdata_per_page;
static HDATA* pages[num_pages];

// these must be signed, because there won't always be a valid
// first or last element.
static i32 first_free = -1;		// don't want to scan array every h_alloc
static i32 last_in_use = -1;	// don't search unused entries


// error checking strategy:
// all handles passed in go through h_data(Handle, Type)


// get an array entry (array is non-contiguous).
// fails (returns 0) if idx is out of bounds, or if accessing a new page
// for the first time, and there's not enough memory to allocate it.
// used by h_data, and alloc_idx to find a free entry.
// beware of conflict with h_data_any_type:
//   our i32 param silently converts to its Handle (= i64) param.
static HDATA* h_data(const i32 idx)
{
	// don't compare against last_in_use - this is called before allocating
	// new entries, and to check if the next (but possibly not yet valid)
	// entry is free. tag check protects against using unallocated entries.
	if(idx < 0 || idx >= hdata_cap)
		return 0;
	HDATA*& page = pages[idx / hdata_per_page];
	if(!page)
	{
		page = (HDATA*)calloc(PAGE_SIZE, 1);
		if(!page)
			return 0;
	}

	return &page[idx % hdata_per_page];
}


// get HDATA for the given handle. verifies the handle
// isn't invalid or an error code, and checks the tag field.
// used by the few functions callable for any handle type, e.g. h_filename.
static HDATA* h_data_any_type(const Handle h)
{
#ifdef PARANOIA
	check_heap();
#endif

	// invalid, or an error code
	if(h <= 0)
		return 0;

	i32 idx = h_idx(h);
	// this function is only called for existing handles.
	// they'd also fail the tag check below, but bail here
	// to avoid needlessly allocating that entry's page.
	if(idx > last_in_use)
		return 0;
	HDATA* hd = h_data(idx);
	if(!hd)
		return 0;

	// note: tag = 0 marks unused entries => is invalid
	u32 tag = h_tag(h);
	if(tag == 0 || tag != hd->tag)
		return 0;

	return hd;
}


// get HDATA for the given handle, also checking handle type.
// used by most functions accessing handle data.
static HDATA* h_data(const Handle h, const H_Type type)
{
	HDATA* hd = h_data_any_type(h);
	if(!hd)
		return 0;

	// h_alloc makes sure type isn't 0, so no need to check that here.
	if(hd->type != type)
		return 0;

	return hd;
}





static void cleanup(void)
{
	// close open handles
	for(i32 i = 0; i < last_in_use; i++)
	{
		HDATA* hd = h_data(i);
		if(hd)
		{
			// somewhat messy, but this only happens on cleanup.
			// better than an additional h_free(i32 idx) version though.
			Handle h = handle(i, hd->tag);
			h_free(h, hd->type);
		}
	}

	// free HDATA array
	for(uint j = 0; j < num_pages; j++)
	{
		free(pages[j]);
		pages[j] = 0;
	}
}




// idx and hd are undefined if we fail.
// called by h_alloc only.
static int alloc_idx(i32& idx, HDATA*& hd)
{
	// we already know the first free entry
	if(first_free != -1)
	{
		idx = first_free;
		hd = h_data(idx);
	}
	// need to look for a free entry, or alloc another
	else
	{
		// look for an unused entry
		for(idx = 0; idx <= last_in_use; idx++)
		{
			hd = h_data(idx);
			assert(hd);	// can't fail - idx is valid

			// found one - done
			if(!hd->tag)
				goto have_idx;
		}

		// add another
		if(last_in_use >= hdata_cap)
		{
			assert(!"alloc_idx: too many open handles (increase IDX_BITS)");
			return -1;
		}
		idx = last_in_use+1;	// incrementing idx would start it at 1
		hd = h_data(idx);
		if(!hd)
			return ERR_NO_MEM;
			// can't fail for any other reason - idx is checked above.
		{	// VC6 goto fix
		bool is_unused = !hd->tag;
		assert(is_unused && "alloc_idx: invalid last_in_use");
		}

have_idx:;
	}

	// check if next entry is free
	HDATA* hd2 = h_data(idx+1);
	if(hd2 && hd2->tag == 0)
		first_free = idx+1;
	else
		first_free = -1;

	if(idx > last_in_use)
		last_in_use = idx;

	return 0;
}


static int free_idx(i32 idx)
{
	if(first_free == -1 || idx < first_free)
		first_free = idx;
	return 0;
}




int h_free(Handle& h, H_Type type)
{
	HDATA* hd = h_data(h, type);
	if(!hd)
		return ERR_INVALID_HANDLE;

	// have valid refcount (don't decrement if alread 0)
	if(hd->refs > 0)
	{
		hd->refs--;
		// not the last reference
		if(hd->refs > 0)
			return 0;
	}

	// TODO: keep this handle open (cache)

	// h_alloc makes sure type != 0; if we get here, it still is
	H_VTbl* vtbl = hd->type;

	// call its destructor
	// note: H_TYPE_DEFINE currently always defines a dtor, but play it safe
	if(vtbl->dtor)
		vtbl->dtor(hd->user);

	memset(hd, 0, sizeof(HDATA));

	i32 idx = h_idx(h);
	free_idx(idx);

	return 0;
}


// any further params are passed to type's init routine
Handle h_alloc(H_Type type, const char* fn, uint flags, ...)
{
	ONCE(atexit(cleanup))

	Handle err;

	i32 idx;
	HDATA* hd;

	// verify type
	if(!type)
	{
		assert(0 && "h_alloc: type param is 0");
		return 0;
	}
	if(type->user_size > HDATA_USER_SIZE)
	{
		assert(0 && "h_alloc: type's user data is too large for HDATA");
		return 0;
	}
	if(type->name == 0)
	{
		assert(0 && "h_alloc: type's name field is 0");
		return 0;
	}

	uintptr_t key = 0;
	// not backed by file; fn is the key
	if(flags & RES_KEY)
	{
		key = (uintptr_t)fn;
		fn = 0;
	}
	else
	{
		if(fn)
			key = fnv_hash(fn, strlen(fn));
	}

	if(key)
	{
/*
		// object already loaded?
		Handle h = h_find(type, key);
		if(h)
		{
			hd = h_data(h, type);
			if(hd->refs == REF_MAX)
			{
				assert(0 && "h_alloc: too many references to a handle - increase REF_BITS");
				return 0;
			}
			hd->refs++;

			return h;
		}
*/
	}

	err = alloc_idx(idx, hd);
	if(err < 0)
		return err;

	static u32 tag;
	if(++tag >= TAG_MASK)
	{
		assert(0 && "h_alloc: tag overflow - allocations are no longer unique."\
		            "may not notice stale handle reuse. increase TAG_BITS.");
		tag = 1;
	}

	hd->key = key;
	hd->tag = tag;
	hd->type = type;
	Handle h = handle(idx, tag);

	H_VTbl* vtbl = type;

	va_list args;
	va_start(args, flags);

	if(vtbl->init)
		vtbl->init(hd->user, args);

	va_end(args);

	if(vtbl->reload)
	{
		err = vtbl->reload(hd->user, fn);
		if(err < 0)
		{
			h_free(h, type);
			return err;
		}
	}

	return h;
}


void* h_user_data(const Handle h, const H_Type type)
{
	HDATA* hd = h_data(h, type);
	return hd? hd->user : 0;
}


const char* h_filename(const Handle h)
{
	HDATA* hd = h_data_any_type(h);
		// don't require type check: should be useable for any handle,
		// even if the caller doesn't know its type.
	return hd? hd->fn : 0;
}


int h_reload(const char* fn)
{
	if(!fn)
	{
		assert(0 && "h_reload: fn = 0");
		return ERR_INVALID_PARAM;
	}

	const u32 key = fnv_hash(fn, strlen(fn));

	i32 i;
	// destroy (note: not free!) all handles backed by this file.
	// do this before reloading any of them, because we don't specify reload
	// order (the parent resource may be reloaded first, and load the child,
	// whose original data would leak).
	for(i = 0; i <= last_in_use; i++)
	{
		HDATA* hd = h_data(i);
		if(hd && hd->key == key)
			hd->type->dtor(hd->user);
	}

	int ret = 0;

	// now reload all affected handles
	// TODO: if too slow
	for(i = 0; i <= last_in_use; i++)
	{
		HDATA* hd = h_data(i);
		if(!hd)
			continue;

		int err = hd->type->reload(hd->user, hd->fn);
		// don't stop if an error is encountered - try to reload them all.
		if(err < 0)
			ret = err;
	}

	return ret;
}




int res_cur_scope;

