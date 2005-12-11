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
#include "lib/allocators.h"

#include <limits.h>	// CHAR_BIT
#include <string.h>
#include <stdlib.h>
#include <new>		// std::bad_alloc
#include "lib/allocators.h"

static const uint MAX_EXTANT_HANDLES = 10000;

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
{
	return (u32)((h >> IDX_SHIFT) & IDX_MASK) - 1;
}

// return the handle's tag field.
// no error checking!
static inline u32 h_tag(const Handle h)
{
	return (u32)((h >> TAG_SHIFT) & TAG_MASK);
}

// build a handle from index and tag.
// can't fail.
static inline Handle handle(const u32 _idx, const u32 tag)
{
	const u32 idx = _idx+1;
	debug_assert(idx <= IDX_MASK && tag <= TAG_MASK && "handle: idx or tag too big");
	// somewhat clunky, but be careful with the shift:
	// *_SHIFT may be larger than its field's type.
	Handle h_idx = idx & IDX_MASK; h_idx <<= IDX_SHIFT;
	Handle h_tag = tag & TAG_MASK; h_tag <<= TAG_SHIFT;
	Handle h = h_idx | h_tag;
	debug_assert(h > 0);
	return h;
}


//
// internal per-resource-instance data
//


// determines maximum number of references to a resource.
static const uint REF_BITS  = 16;
static const u32 REF_MAX = (1ul << REF_BITS)-1;

static const uint TYPE_BITS = 8;


// chosen so that all current resource structs are covered,
// and so sizeof(HDATA) is a power of 2 (for more efficient array access
// and array page usage).
static const size_t HDATA_USER_SIZE = 44+64;


struct HDATA
{
	uintptr_t key;

	u32 tag  : TAG_BITS;

	// smaller bitfields combined into 1
	u32 refs : REF_BITS;
	u32 type_idx : TYPE_BITS;
	// .. if set, do not actually release the resource (i.e. call dtor)
	//    when the handle is h_free-d, regardless of the refcount.
	//    set by h_alloc; reset on exit and by housekeeping.
	u32 keep_open : 1;
	// .. HACK: prevent adding to h_find lookup index if flags & RES_UNIQUE
	//    (because those handles might have several instances open,
	//    which the index can't currently handle)
	u32 unique : 1;
	u32 disallow_reload : 1;

	H_Type type;

	// for statistics
	uint num_derefs;
	
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


// get a (possibly new) array entry; array is non-contiguous.
//
// fails (returns 0) if idx is out of bounds, or if accessing a new page
// for the first time, and there's not enough memory to allocate it.
//
// also used by h_data, and alloc_idx to find a free entry.
static HDATA* h_data_from_idx(const i32 idx)
{
	// makes things *crawl*!
#if CONFIG_PARANOIA
	debug_heap_check();
#endif

	// don't compare against last_in_use - this is called before allocating
	// new entries, and to check if the next (but possibly not yet valid)
	// entry is free. tag check protects against using unallocated entries.
	if(idx < 0 || idx >= hdata_cap)
		return 0;
	HDATA*& page = pages[idx / hdata_per_page];
	if(!page)
	{
		page = (HDATA*)calloc(1, PAGE_SIZE);
		if(!page)
			return 0;
	}

	// note: VC7.1 optimizes the divides to shift and mask.

	HDATA* hd = &page[idx % hdata_per_page];
	hd->num_derefs++;
	return hd;
}


// get HDATA for the given handle.
// only uses (and checks) the index field.
// used by h_force_close (which must work regardless of tag).
static inline HDATA* h_data_no_tag(const Handle h)
{
	i32 idx = (i32)h_idx(h);
	// need to verify it's in range - h_data_from_idx can only verify that
	// it's < maximum allowable index.
	if(0 > idx || idx > last_in_use)
		return 0;
	return h_data_from_idx(idx);
}


// get HDATA for the given handle.
// also verifies the tag field.
// used by functions callable for any handle type, e.g. h_filename.
static inline HDATA* h_data_tag(const Handle h)
{
	HDATA* hd = h_data_no_tag(h);
	if(!hd)
		return 0;

	// note: tag = 0 marks unused entries => is invalid
	u32 tag = h_tag(h);
	if(tag == 0 || tag != hd->tag)
		return 0;

	return hd;
}


// get HDATA for the given handle.
// also verifies the type.
// used by most functions accessing handle data.
static HDATA* h_data_tag_type(const Handle h, const H_Type type)
{
	HDATA* hd = h_data_tag(h);
	if(!hd)
		return 0;

	// h_alloc makes sure type isn't 0, so no need to check that here.
	if(hd->type != type)
		return 0;

	return hd;
}


//-----------------------------------------------------------------------------

// idx and hd are undefined if we fail.
// called by h_alloc only.
static LibError alloc_idx(i32& idx, HDATA*& hd)
{
	// we already know the first free entry
	if(first_free != -1)
	{
		idx = first_free;
		hd = h_data_from_idx(idx);
	}
	// need to look for a free entry, or alloc another
	else
	{
		// look for an unused entry
		for(idx = 0; idx <= last_in_use; idx++)
		{
			hd = h_data_from_idx(idx);
			debug_assert(hd);	// can't fail - idx is valid

			// found one - done
			if(!hd->tag)
				goto have_idx;
		}

		// add another
		if(last_in_use >= hdata_cap)
		{
			debug_warn("too many open handles (increase IDX_BITS)");
			return ERR_LIMIT;
		}
		idx = last_in_use+1;	// just incrementing idx would start it at 1
		hd = h_data_from_idx(idx);
		if(!hd)
			return ERR_NO_MEM;
			// can't fail for any other reason - idx is checked above.
		{	// VC6 goto fix
		bool is_unused = !hd->tag;
		debug_assert(is_unused && "invalid last_in_use");
		}

have_idx:;
	}

	// check if next entry is free
	HDATA* hd2 = h_data_from_idx(idx+1);
	if(hd2 && hd2->tag == 0)
		first_free = idx+1;
	else
		first_free = -1;

	if(idx > last_in_use)
		last_in_use = idx;

	return ERR_OK;
}


static LibError free_idx(i32 idx)
{
	if(first_free == -1 || idx < first_free)
		first_free = idx;
	return ERR_OK;
}


//-----------------------------------------------------------------------------
// lookup data structure
//-----------------------------------------------------------------------------

// speed up h_find (called every h_alloc)
// multimap, because we want to add handles of differing type but same key
// (e.g. a VFile and Tex object for the same underlying filename hash key)
//
// store index because it's smaller and Handle can easily be reconstructed
//
//
// note: there may be several RES_UNIQUE handles of the same type and key
// (e.g. sound files - several instances of a sound definition file).
// that wasn't forseen here, so we'll just refrain from adding to the index.
// that means they won't be found via h_find - no biggie.

typedef STL_HASH_MULTIMAP<uintptr_t, i32> Key2Idx;
typedef Key2Idx::iterator It;
static OverrunProtector<Key2Idx> key2idx_wrapper;

enum KeyRemoveFlag { KEY_NOREMOVE, KEY_REMOVE };

static Handle key_find(uintptr_t key, H_Type type, KeyRemoveFlag remove_option = KEY_NOREMOVE)
{
	Key2Idx* key2idx = key2idx_wrapper.get();
	if(!key2idx)
		return ERR_NO_MEM;

	// initial return value: "not found at all, or it's of the
	// wrong type". the latter happens when called by h_alloc to
	// check if e.g. a Tex object already exists; at that time,
	// only the corresponding VFile exists.
	Handle ret = -1;

	std::pair<It, It> range = key2idx->equal_range(key);
	for(It it = range.first; it != range.second; ++it)
	{
		i32 idx = it->second;
		HDATA* hd = h_data_from_idx(idx);
		// found match
		if(hd && hd->type == type && hd->key == key)
		{
			if(remove_option == KEY_REMOVE)
				key2idx->erase(it);
			ret = handle(idx, hd->tag);
			break;
		}
	}

	key2idx_wrapper.lock();
	return ret;
}


static void key_add(uintptr_t key, Handle h)
{
	Key2Idx* key2idx = key2idx_wrapper.get();
	if(!key2idx)
		return;

	const i32 idx = h_idx(h);
	// note: MSDN documentation of stdext::hash_multimap is incorrect;
	// there is no overload of insert() that returns pair<iterator, bool>.
	(void)key2idx->insert(std::make_pair(key, idx));

	key2idx_wrapper.lock();
}


static void key_remove(uintptr_t key, H_Type type)
{
	Handle ret = key_find(key, type, KEY_REMOVE);
	debug_assert(ret > 0);
}


//-----------------------------------------------------------------------------

//
// path string suballocator (they're stored in HDATA)
//

static Pool fn_pool;


// if fn is longer than this, it has to be allocated from the heap.
// choose this to balance internal fragmentation and accessing the heap.
static const size_t FN_POOL_EL_SIZE = 64;

static void fn_init()
{
	// (if this fails, so will subsequent fn_stores - no need to complain here)
	(void)pool_create(&fn_pool, MAX_EXTANT_HANDLES*FN_POOL_EL_SIZE, FN_POOL_EL_SIZE);
}

static void fn_store(HDATA* hd, const char* fn)
{
	ONCE(fn_init());

	const size_t size = strlen(fn)+1;

	hd->fn = 0;
	// stuff it in unused space at the end of HDATA
	if(hd->type->user_size+size <= HDATA_USER_SIZE)
		hd->fn = (const char*)hd->user + hd->type->user_size;
	else if(size <= FN_POOL_EL_SIZE)
		hd->fn = (const char*)pool_alloc(&fn_pool);

	// in case none of the above applied and/or were successful:
	// fall back to heap alloc.
	if(!hd->fn)
	{
		debug_printf("H_MGR| very long filename (%d) %s\n", size, fn);
		hd->fn = (const char*)malloc(size);
		// still failed - bail (avoid strcpy to 0)
		if(!hd->fn)
		{
			debug_warn("not enough memory to store hd->fn!");
			return;
		}
	}

	strcpy((char*)hd->fn, fn);
}

// TODO: store this in a flag - faster.
static bool fn_is_in_HDATA(HDATA* hd)
{
	u8* p = (u8*)hd->fn;	// needed for type-correct comparison
	return (hd->user+hd->type->user_size <= p && p <= hd->user+HDATA_USER_SIZE);
}

static void fn_free(HDATA* hd)
{
	void* el = (void*)hd->fn;
	if(fn_is_in_HDATA(hd))
	{
	}
	else if(pool_contains(&fn_pool, el))
		pool_free(&fn_pool, el);
	else
		free(el);
}

static void fn_shutdown()
{
	pool_destroy(&fn_pool);
}


//----------------------------------------------------------------------------
// h_alloc
//----------------------------------------------------------------------------


static void warn_if_invalid(HDATA* hd)
{
#ifndef NDEBUG
	H_VTbl* vtbl = hd->type;

	// validate HDATA
	// currently nothing to do; <type> is checked by h_alloc and
	// the others have no invariants we could check.

	// have the resource validate its user_data
	LibError err = vtbl->validate(hd->user);
	debug_assert(err == ERR_OK);

	// make sure empty space in control block isn't touched
	// .. but only if we're not storing a filename there
	if(!fn_is_in_HDATA(hd))
	{
		const u8* start = hd->user + vtbl->user_size;
		const u8* end   = hd->user + HDATA_USER_SIZE;
		for(const u8* p = start; p < end; p++)
			if(*p != 0)
				debug_warn("handle user data was overrun!");
	}
#else
	UNUSED2(hd);
#endif
}


static LibError type_validate(H_Type type)
{
	if(!type)
	{
		debug_warn("type is 0");
		return ERR_INVALID_PARAM;
	}
	if(type->user_size > HDATA_USER_SIZE)
	{
		debug_warn("type's user data is too large for HDATA");
		return ERR_LIMIT;
	}
	if(type->name == 0)
	{
		debug_warn("type's name field is 0");
		return ERR_INVALID_PARAM;
	}

	return ERR_OK;
}


static u32 gen_tag()
{
	static u32 tag;
	if(++tag >= TAG_MASK)
	{
		debug_warn("h_mgr: tag overflow - allocations are no longer unique."\
			"may not notice stale handle reuse. increase TAG_BITS.");
		tag = 1;
	}
	return tag;
}


static Handle reuse_existing_handle(uintptr_t key, H_Type type, uint flags)
{
	if(flags & RES_NO_CACHE)
		return 0;

	// object of specified key and type doesn't exist yet
	Handle h = h_find(type, key);
	if(h <= 0)	// "failure" is meaningless; we only care if found or not
		return 0;

	HDATA* hd = h_data_tag_type(h, type);
	if(hd->refs == REF_MAX)
	{
		debug_warn("too many references to a handle - increase REF_BITS");
		return ERR_LIMIT;
	}

	hd->refs++;

	// we are reactivating a closed but cached handle.
	// need to generate a new tag so that copies of the
	// previous handle can no longer access the resource.
	// (we don't need to reset the tag in h_free, because
	// use before this fails due to refs > 0 check in h_user_data).
	if(hd->refs == 1)
	{
		const u32 tag = gen_tag();
		hd->tag = tag;
		h = handle(h_idx(h), tag);	// can't fail
	}

	return h;
}


static LibError call_init_and_reload(Handle h, H_Type type, HDATA* hd, const char* fn, va_list* init_args)
{
	LibError err = ERR_OK;
	H_VTbl* vtbl = type;	// exact same thing but for clarity

	// init
	if(vtbl->init)
		vtbl->init(hd->user, *init_args);

	// reload
	if(vtbl->reload)
	{
		// catch exception to simplify reload funcs - let them use new()
		try
		{
			err = vtbl->reload(hd->user, fn, h);
			if(err == ERR_OK)
				warn_if_invalid(hd);
		}
		catch(std::bad_alloc)
		{
			err = ERR_NO_MEM;
		}
	}

	return err;
}


static Handle alloc_new_handle(H_Type type, const char* fn, uintptr_t key,
	uint flags, va_list* init_args)
{
	i32 idx;
	HDATA* hd;
	CHECK_ERR(alloc_idx(idx, hd));

	// (don't want to do this before the add-reference exit,
	// so as not to waste tags for often allocated handles.)
	const u32 tag = gen_tag();
	Handle h = handle(idx, tag);	// can't fail.

	hd->tag  = tag;
	hd->key  = key;
	hd->type = type;
	hd->refs = 1;
	if(!(flags & RES_NO_CACHE))
		hd->keep_open = 1;
	if(flags & RES_DISALLOW_RELOAD)
		hd->disallow_reload = 1;
	hd->unique = (flags & RES_UNIQUE) != 0;
	hd->fn = 0;
	// .. filename is valid - store in hd
	// note: if the original fn param was a key, it was reset to 0 above.
	if(fn)
		fn_store(hd, fn);

	if(key && !hd->unique)
		key_add(key, h);

	LibError err = call_init_and_reload(h, type, hd, fn, init_args);
	if(err < 0)
		goto fail;

	return h;

fail:
	// reload failed; free the handle
	hd->keep_open = 0;	// disallow caching (since contents are invalid)
	(void)h_free(h, type);	// (h_free already does WARN_ERR)

	// note: since some uses will always fail (e.g. loading sounds if
	// g_Quickstart), do not complain here.
	return (Handle)err;
}


// any further params are passed to type's init routine
Handle h_alloc(H_Type type, const char* fn, uint flags, ...)
{
	CHECK_ERR(type_validate(type));

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

//debug_printf("alloc %s %s\n", type->name, fn);

	// no key => can never be found. disallow caching
	if(!key)
		flags |= RES_NO_CACHE;

	// see if we can reuse an existing handle
	Handle h = reuse_existing_handle(key, type, flags);
	// .. error
	CHECK_ERR(h);
	// .. successfully reused the handle; refcount increased
	if(h > 0)
		return h;
	// .. need to allocate a new one:
	va_list args;
	va_start(args, flags);
	h = alloc_new_handle(type, fn, key, flags, &args);
	va_end(args);
	return h;	// alloc_new_handle already does CHECK_ERR
}


//-----------------------------------------------------------------------------

// currently cannot fail.
static LibError h_free_idx(i32 idx, HDATA* hd)
{
	// debug_printf("free %s %s\n", type->name, hd->fn);

	// only decrement if refcount not already 0.
	if(hd->refs > 0)
		hd->refs--;

	// still references open or caching requests it stays - do not release.
	if(hd->refs > 0 || hd->keep_open)
		return ERR_OK;

	// actually release the resource (call dtor, free control block).

	// h_alloc makes sure type != 0; if we get here, it still is
	H_VTbl* vtbl = hd->type;

	// call its destructor
	// note: H_TYPE_DEFINE currently always defines a dtor, but play it safe
	if(vtbl->dtor)
		vtbl->dtor(hd->user);

	if(hd->key && !hd->unique)
		key_remove(hd->key, hd->type);

	// get pretty version of filename: start with "not applicable"
	const char* fn = "(0)";
	if(hd->fn)
	{
		// if hd->fn is a filename, strip the path. note: some paths end
		// with '/', so display those unaltered.
		const char* slash = strrchr(hd->fn, '/');
		fn = (slash && slash[1] != '\0')? slash+1 : hd->fn;
	}

#ifndef NDEBUG
	char buf[H_STRING_LEN];
	if(vtbl->to_string(hd->user, buf) < 0)
		strcpy(buf, "(error)");	// safe
	debug_printf("H_MGR| free %s %s accesses=%d %s\n", hd->type->name, fn, hd->num_derefs, buf);
#endif

	fn_free(hd);

	memset(hd, 0, sizeof(*hd));

	free_idx(idx);

	return ERR_OK;
}


LibError h_free(Handle& h, H_Type type)
{
	i32 idx = h_idx(h);
	HDATA* hd = h_data_tag_type(h, type);

	// wipe out the handle to prevent reuse but keep a copy for below.
	const Handle h_copy = h;
	h = 0;

	// h was invalid
	if(!hd)
	{
		// 0-initialized or an error code; don't complain because this
		// happens often and is harmless.
		if(h_copy <= 0)
			return ERR_OK;
		// this was a valid handle but was probably freed in the meantime.
		// complain because this probably indicates a bug somewhere.
		CHECK_ERR(ERR_INVALID_HANDLE);
	}

	return h_free_idx(idx, hd);
}


//----------------------------------------------------------------------------
// remaining API

void* h_user_data(const Handle h, const H_Type type)
{
	HDATA* hd = h_data_tag_type(h, type);
	if(!hd)
		return 0;

	if(!hd->refs)
	{
		// note: resetting the tag is not enough (user might pass in its value)
		debug_warn("no references to resource (it's cached, but someone is accessing it directly)");
		return 0;
	}

	warn_if_invalid(hd);
	return hd->user;
}


const char* h_filename(const Handle h)
{
	// don't require type check: should be useable for any handle,
	// even if the caller doesn't know its type.
	HDATA* hd = h_data_tag(h);
	if(!hd)
	{
		debug_warn("failed");
		return 0;
	}
	return hd->fn;
}


// TODO: what if iterating through all handles is too slow?
LibError h_reload(const char* fn)
{
	if(!fn)
	{
		debug_warn("fn = 0");
		// must not continue - some resources not backed by files have
		// key = 0 and reloading those would be disastrous.
		return ERR_INVALID_PARAM;
	}

	const u32 key = fnv_hash(fn);

	// destroy (note: not free!) all handles backed by this file.
	// do this before reloading any of them, because we don't specify reload
	// order (the parent resource may be reloaded first, and load the child,
	// whose original data would leak).
	for(i32 i = 0; i <= last_in_use; i++)
	{
		HDATA* hd = h_data_from_idx(i);
		if(!hd || hd->key != key || hd->disallow_reload)
			continue;
		hd->type->dtor(hd->user);
	}

	LibError ret = ERR_OK;

	// now reload all affected handles
	for(i32 i = 0; i <= last_in_use; i++)
	{
		HDATA* hd = h_data_from_idx(i);
		if(!hd || hd->key != key || hd->disallow_reload)
			continue;

		Handle h = handle(i, hd->tag);

		LibError err = hd->type->reload(hd->user, hd->fn, h);
		// don't stop if an error is encountered - try to reload them all.
		if(err < 0)
		{
			h_free(h, hd->type);
			if(ret == 0)	// don't overwrite first error
				ret = err;
		}
		else
			warn_if_invalid(hd);
	}

	return ret;
}


Handle h_find(H_Type type, uintptr_t key)
{
	return key_find(key, type);
}



// force the resource to be freed immediately, even if cached.
// tag is not checked - this allows the first Handle returned
// (whose tag will change after being 'freed', but remaining in memory)
// to later close the object.
// this is used when reinitializing the sound engine -
// at that point, all (cached) OpenAL resources must be freed.
LibError h_force_free(Handle h, H_Type type)
{
	// require valid index; ignore tag; type checked below.
	HDATA* hd = h_data_no_tag(h);
	if(!hd || hd->type != type)
		return ERR_INVALID_HANDLE;
	u32 idx = h_idx(h);
	hd->keep_open = 0;
	hd->refs = 0;
	return h_free_idx(idx, hd);
}


// increment Handle <h>'s reference count.
// only meant to be used for objects that free a Handle in their dtor,
// so that they are copy-equivalent and can be stored in a STL container.
// do not use this to implement refcounting on top of the Handle scheme,
// e.g. loading a Handle once and then passing it around. instead, have each
// user load the resource; refcounting is done under the hood.
void h_add_ref(Handle h)
{
	HDATA* hd = h_data_tag(h);
	if(!hd)
	{
		debug_warn("invalid handle");
		return;
	}

	// if there are no refs, how did the caller manage to keep a Handle?!
	if(!hd->refs)
		debug_warn("no refs open - resource is cached");

	hd->refs++;
}


// retrieve the internal reference count or a negative error code.
// background: since h_alloc has no way of indicating whether it
// allocated a new handle or reused an existing one, counting references
// within resource control blocks is impossible. since that is sometimes
// necessary (always wrapping objects in Handles is excessive), we
// provide access to the internal reference count.
int h_get_refcnt(Handle h)
{
	HDATA* hd = h_data_tag(h);
	if(!hd)
	{
		debug_warn("invalid handle");
		return ERR_INVALID_PARAM;
	}

	// if there are no refs, how did the caller manage to keep a Handle?!
	if(!hd->refs)
		debug_warn("no refs open - resource is cached");

	return hd->refs;
}


void h_mgr_shutdown()
{
	debug_printf("==h_mgr_shutdown== (all handle frees after this are leaks)\n");

	// forcibly close all open handles
	for(i32 i = 0; i <= last_in_use; i++)
	{
		HDATA* hd = h_data_from_idx(i);
		// can't fail - i is in bounds by definition, and
		// each HDATA entry has already been allocated.
		if(!hd)
		{
			debug_warn("h_data_from_idx failed - why?!");
			continue;
		}

		// it's already been freed; don't free again so that this
		// doesn't look like an error.
		if(!hd->tag)
			continue;

		// disable caching; we need to release the resource now.
		hd->keep_open = 0;
		hd->refs = 0;

		h_free_idx(i, hd);	// currently cannot fail
	}

	// free HDATA array
	for(uint j = 0; j < num_pages; j++)
	{
		free(pages[j]);
		pages[j] = 0;
	}

	fn_shutdown();
}
