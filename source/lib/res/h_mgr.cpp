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

#include "precompiled.h"

#include "lib.h"
#include "h_mgr.h"
#include "mem.h"

#include <assert.h>
#include <limits.h>	// CHAR_BIT
#include <string.h>
#include <stdlib.h>
#include <new> // std::bad_alloc


// rationale
//
// why fixed size control blocks, instead of just allocating dynamically?
// it is expected that resources be created and freed often. this way is
// much nicer to the memory manager. defining control blocks larger than
// the allotted space is caught by h_alloc (made possible by the vtbl builder
// storing control block size). it is also efficient to have all CBs in an
// more or less contiguous array (see below).
//
// why a manager, instead of a simple pool allocator?
// we need a central list of resources for freeing at exit, checking if a
// resource has already been loaded (for caching), and when reloading.
// may as well keep them in an array, rather than add a list and index.



//
// handle
//

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

// - index (0-based) of control block in our array.
//   (field width determines maximum currently open handles)
#define IDX_BITS 16
const uint IDX_SHIFT = 32;
const u32 IDX_MASK = (1l << IDX_BITS) - 1;

// make sure both fields fit within a Handle variable
cassert(IDX_BITS + TAG_BITS <= sizeof(Handle)*CHAR_BIT);


// return the handle's index field (always non-negative).
// no error checking!
static inline u32 h_idx(const Handle h)
{ return (u32)((h >> IDX_SHIFT) & IDX_MASK) - 1; }

// return the handle's tag field.
// no error checking!
static inline u32 h_tag(const Handle h)
{ return (u32)((h >> TAG_SHIFT) & TAG_MASK); }

// build a handle from index and tag.
// can't fail.
static inline Handle handle(const u32 _idx, const u32 tag)
{
	const u32 idx = _idx+1;
	assert(idx <= IDX_MASK && tag <= TAG_MASK && "handle: idx or tag too big");
	// somewhat clunky, but be careful with the shift:
	// *_SHIFT may be larger than its field's type.
	Handle h_idx = idx & IDX_MASK; h_idx <<= IDX_SHIFT;
	Handle h_tag = tag & TAG_MASK; h_tag <<= TAG_SHIFT;
	Handle h = h_idx | h_tag;
	assert(h > 0);
	return h;
}


//
// internal per-resource-instance data
//


// determines maximum number of references to a resource.
static const uint REF_BITS  = 8;
static const u32 REF_MAX = (1ul << REF_BITS)-1;

static const uint TYPE_BITS = 8;

// a handle's idx field isn't stored in its HDATA entry (not needed);
// to save space, these should take its place, i.e. they should fit in IDX_BITS.
// if not, ignore + comment out this assertion.
cassert(REF_BITS + TYPE_BITS <= IDX_BITS);


// chosen so that all current resource structs are covered,
// and so sizeof(HDATA) is a power of 2 (for more efficient array access
// and array page usage).
static const size_t HDATA_USER_SIZE = 44+64;


// 64 bytes
// TODO: not anymore, fix later
struct HDATA
{
	uintptr_t key;

	u32 tag  : TAG_BITS;

	// smaller bitfields combined into 1
	u32 refs : REF_BITS;
	u32 type_idx : TYPE_BITS;
	u32 keep_open : 1;
		// regardless of refs, do not actually release the resource
		// (i.e. call dtor) when the handle is h_free-d.
		// set by h_alloc; reset on exit and by housekeeping.

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
//
// fails (returns 0) if idx is out of bounds, or if accessing a new page
// for the first time, and there's not enough memory to allocate it.
//
// also used by h_data, and alloc_idx to find a free entry.
//
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
	// they'd also fail the tag check below, but bail out here
	// already to avoid needlessly allocating that entry's page.
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





void h_mgr_shutdown(void)
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

			// disable caching; we need to release the resource now.
			hd->keep_open = 0;

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
		idx = last_in_use+1;	// just incrementing idx would start it at 1
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

	// only decrement if refcount not already 0.
	if(hd->refs > 0)
		hd->refs--;

	// still references open or caching requests it stays - do not release.
	if(hd->refs > 0 || hd->keep_open)
		return 0;

	// actually release the resource (call dtor, free control block).

	// h_alloc makes sure type != 0; if we get here, it still is
	H_VTbl* vtbl = hd->type;

	// call its destructor
	// note: H_TYPE_DEFINE currently always defines a dtor, but play it safe
	if(vtbl->dtor)
		vtbl->dtor(hd->user);

	free((void*)hd->fn);

	memset(hd, 0, sizeof(HDATA));

	i32 idx = h_idx(h);
	free_idx(idx);

	return 0;
}


static int type_validate(H_Type type)
{
	int err = ERR_INVALID_PARAM;

	if(!type)
	{
		debug_warn("h_alloc: type is 0");
		goto fail;
	}
	if(type->user_size > HDATA_USER_SIZE)
	{
		debug_warn("h_alloc: type's user data is too large for HDATA");
		goto fail;
	}
	if(type->name == 0)
	{
		debug_warn("h_alloc: type's name field is 0");
		goto fail;
	}

	// success
	err = 0;

fail:
	return err;
}


// any further params are passed to type's init routine
Handle h_alloc(H_Type type, const char* fn, uint flags, ...)
{
	ONCE(atexit2(h_mgr_shutdown));

	CHECK_ERR(type_validate(type));

	Handle h = 0;
	i32 idx;
	HDATA* hd;

	const uint scope = flags & RES_SCOPE_MASK;

	// get key (either hash of filename, or fn param)
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
			key = fnv_hash(fn);
	}

	if(key)
	{
		// object already loaded?
		h = h_find(type, key);
		if(h > 0)
		{
			hd = h_data(h, type);
			if(hd->refs == REF_MAX)
			{
				debug_warn("h_alloc: too many references to a handle - increase REF_BITS");
				return 0;
			}
			hd->refs++;

			// adding reference; already in-use
			if(hd->refs > 1)
			{
//				debug_warn("adding reference to handle (undermines tag security check)");
				return h;
			}

			// we re-activated a cached resource. will reset tag below, then bail.
		}
	}

	// generate next tag value.
	// don't want to do this before the add-reference exit,
	// so as not to waste tags for often allocated handles.
	static u32 tag;
	if(++tag >= TAG_MASK)
	{
		debug_warn("h_alloc: tag overflow - allocations are no longer unique."\
			"may not notice stale handle reuse. increase TAG_BITS.");
		tag = 1;
	}

	// we are reactivating a closed but cached handle.
	// need to reset tag, so that copies of the previous
	// handle can no longer access the resource.
	// (we don't need to reset the tag in h_free, because
	// use before this fails due to refs > 0 check in h_user_data).
	if(h > 0)
	{
		hd->tag = tag;
		return h;
	}

	CHECK_ERR(alloc_idx(idx, hd));

	// one-time init
	hd->tag  = tag;
	hd->key  = key;
	hd->type = type;
	hd->refs = 1;
	if(scope != RES_TEMP)
		hd->keep_open = 1;
	// .. filename is valid - store in hd
	// note: if the original fn param was a key, it was reset to 0 above.
	if(fn)
	{
		const size_t fn_len = strlen(fn);
		hd->fn = (const char*)malloc(fn_len+1);
		strcpy((char*)hd->fn, fn);
	}
	else
		hd->fn = 0;

	h = handle(idx, tag);
		// can't fail.

	H_VTbl* vtbl = type;

	// init
	va_list args;
	va_start(args, flags);
	if(vtbl->init)
		vtbl->init(hd->user, args);
	va_end(args);

	// reload
	if(vtbl->reload)
	{
		// catch exception to simplify reload funcs - let them use new()
		int err;
		try
		{
			err = vtbl->reload(hd->user, fn, h);
		}
		catch(std::bad_alloc)
		{
			err = ERR_NO_MEM;
		}

		if(err < 0)
		{
			h_free(h, type);
			return (Handle)err;
		}
	}

	return h;
}


void* h_user_data(const Handle h, const H_Type type)
{
	HDATA* hd = h_data(h, type);
	if(!hd)
		return 0;

	if(!hd->refs)
	{
		// note: resetting the tag is not enough (user might pass in its value)
		debug_warn("h_user_data: no references to resource (it's cached, but someone is accessing it directly)");
		return 0;
	}

	return hd->user;
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
		debug_warn("h_reload: fn = 0");
		return ERR_INVALID_PARAM;
	}

	const u32 key = fnv_hash(fn);

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
	// TODO: what if too slow to iterate through all handles?
	for(i = 0; i <= last_in_use; i++)
	{
		HDATA* hd = h_data(i);
		if(!hd || hd->key != key)
			continue;

		Handle h = handle(i, hd->tag);

		int err = hd->type->reload(hd->user, hd->fn, h);
		// don't stop if an error is encountered - try to reload them all.
		if(err < 0)
		{
			h_free(h, hd->type);
			ret = err;
		}
	}

	return ret;
}


// TODO: more efficient search; currently linear
Handle h_find(H_Type type, uintptr_t key)
{
	for(i32 i = 0; i < last_in_use; i++)
	{
		HDATA* hd = h_data(i);
		if(hd)
			if(hd->type == type && hd->key == key)
				return handle(i, hd->tag);
	}

	return -1;
}



int res_cur_scope;

