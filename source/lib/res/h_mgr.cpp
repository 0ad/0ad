/* Copyright (C) 2013 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * handle manager for resources.
 */

#include "precompiled.h"
#include "h_mgr.h"

#include <boost/unordered_map.hpp>

#include <limits.h>	// CHAR_BIT
#include <string.h>
#include <stdlib.h>
#include <new>		// std::bad_alloc

#include "lib/fnv_hash.h"
#include "lib/allocators/overrun_protector.h"
#include "lib/allocators/pool.h"
#include "lib/module_init.h"

#include <mutex>

namespace ERR {
static const Status H_IDX_INVALID   = -120000;	// totally invalid
static const Status H_IDX_UNUSED    = -120001;	// beyond current cap
static const Status H_TAG_MISMATCH  = -120003;
static const Status H_TYPE_MISMATCH = -120004;
static const Status H_ALREADY_FREED = -120005;
}
static const StatusDefinition hStatusDefinitions[] = {
	{ ERR::H_IDX_INVALID,   L"Handle index completely out of bounds" },
	{ ERR::H_IDX_UNUSED,    L"Handle index exceeds high-water mark" },
	{ ERR::H_TAG_MISMATCH,  L"Handle tag mismatch (stale reference?)" },
	{ ERR::H_TYPE_MISMATCH, L"Handle type mismatch" },
	{ ERR::H_ALREADY_FREED, L"Handle already freed" }
};
STATUS_ADD_DEFINITIONS(hStatusDefinitions);



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

// - index (0-based) of control block in our array.
//   (field width determines maximum currently open handles)
#define IDX_BITS 16
static const u64 IDX_MASK = (1l << IDX_BITS) - 1;

// - tag (1-based) ensures the handle references a certain resource instance.
//   (field width determines maximum unambiguous resource allocs)
typedef i64 Tag;
#define TAG_BITS 48
static const u64 TAG_MASK = 0xFFFFFFFF;	// safer than (1 << 32) - 1

// make sure both fields fit within a Handle variable
cassert(IDX_BITS + TAG_BITS <= sizeof(Handle)*CHAR_BIT);


// return the handle's index field (always non-negative).
// no error checking!
static inline size_t h_idx(const Handle h)
{
	return (size_t)(h & IDX_MASK) - 1;
}

// return the handle's tag field.
// no error checking!
static inline Tag h_tag(Handle h)
{
	return h >> IDX_BITS;
}

// build a handle from index and tag.
// can't fail.
static inline Handle handle(size_t idx, u64 tag)
{
	const size_t idxPlusOne = idx+1;
	ENSURE(idxPlusOne <= IDX_MASK);
	ENSURE((tag & IDX_MASK) == 0);
	Handle h = tag | idxPlusOne;
	ENSURE(h > 0);
	return h;
}


//
// internal per-resource-instance data
//

// chosen so that all current resource structs are covered.
static const size_t HDATA_USER_SIZE = 104;


struct HDATA
{
	// we only need the tag, because it is trivial to compute
	// &HDATA from idx and vice versa. storing the entire handle
	// avoids needing to extract the tag field.
	Handle h;	// NB: will be overwritten by pool_free

	uintptr_t key;

	intptr_t refs;

	// smaller bit fields combined into 1
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
	size_t num_derefs;

	// storing PIVFS here is not a good idea since this often isn't
	// `freed' due to caching (and there is no dtor), so
	// the VFS reference count would never reach zero.
	VfsPath pathname;

	u8 user[HDATA_USER_SIZE];
};


// max data array entries. compared to last_in_use => signed.
static const ssize_t hdata_cap = (1ul << IDX_BITS)/4;

// pool of fixed-size elements allows O(1) alloc and free;
// there is a simple mapping between HDATA address and index.
static Pool hpool;


// error checking strategy:
// all handles passed in go through h_data(Handle, Type)


// get a (possibly new) array entry.
//
// fails if idx is out of bounds.
static Status h_data_from_idx(ssize_t idx, HDATA*& hd)
{
	// don't check if idx is beyond the current high-water mark, because
	// we might be allocating a new entry. subsequent tag checks protect
	// against using unallocated entries.
	if(size_t(idx) >= size_t(hdata_cap))	// also detects negative idx
		WARN_RETURN(ERR::H_IDX_INVALID);

	hd = (HDATA*)(hpool.da.base + idx*hpool.el_size);
	hd->num_derefs++;
	return INFO::OK;
}

static ssize_t h_idx_from_data(HDATA* hd)
{
	if(!pool_contains(&hpool, hd))
		WARN_RETURN(ERR::INVALID_POINTER);
	return (uintptr_t(hd) - uintptr_t(hpool.da.base))/hpool.el_size;
}


// get HDATA for the given handle.
// only uses (and checks) the index field.
// used by h_force_close (which must work regardless of tag).
static inline Status h_data_no_tag(const Handle h, HDATA*& hd)
{
	ssize_t idx = (ssize_t)h_idx(h);
	RETURN_STATUS_IF_ERR(h_data_from_idx(idx, hd));
	// need to verify it's in range - h_data_from_idx can only verify that
	// it's < maximum allowable index.
	if(uintptr_t(hd) > uintptr_t(hpool.da.base)+hpool.da.pos)
		WARN_RETURN(ERR::H_IDX_UNUSED);
	return INFO::OK;
}


static bool ignoreDoubleFree = false;

// get HDATA for the given handle.
// also verifies the tag field.
// used by functions callable for any handle type, e.g. h_filename.
static inline Status h_data_tag(Handle h, HDATA*& hd)
{
	RETURN_STATUS_IF_ERR(h_data_no_tag(h, hd));

	if(hd->key == 0)	// HDATA was wiped out and hd->h overwritten by pool_free
	{
		if(ignoreDoubleFree)
			return ERR::H_ALREADY_FREED;	// NOWARN (see ignoreDoubleFree)
		else
			WARN_RETURN(ERR::H_ALREADY_FREED);
	}

	if(h != hd->h)
		WARN_RETURN(ERR::H_TAG_MISMATCH);

	return INFO::OK;
}


// get HDATA for the given handle.
// also verifies the type.
// used by most functions accessing handle data.
static Status h_data_tag_type(const Handle h, const H_Type type, HDATA*& hd)
{
	RETURN_STATUS_IF_ERR(h_data_tag(h, hd));

	// h_alloc makes sure type isn't 0, so no need to check that here.
	if(hd->type != type)
	{
		debug_printf("h_mgr: expected type %s, got %s\n", utf8_from_wstring(hd->type->name).c_str(), utf8_from_wstring(type->name).c_str());
		WARN_RETURN(ERR::H_TYPE_MISMATCH);
	}

	return INFO::OK;
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
// that wasn't foreseen here, so we'll just refrain from adding to the index.
// that means they won't be found via h_find - no biggie.

typedef boost::unordered_multimap<uintptr_t, ssize_t> Key2Idx;
typedef Key2Idx::iterator It;
static OverrunProtector<Key2Idx> key2idx_wrapper;

enum KeyRemoveFlag { KEY_NOREMOVE, KEY_REMOVE };

static Handle key_find(uintptr_t key, H_Type type, KeyRemoveFlag remove_option = KEY_NOREMOVE)
{
	Key2Idx* key2idx = key2idx_wrapper.get();
	if(!key2idx)
		WARN_RETURN(ERR::NO_MEM);

	// initial return value: "not found at all, or it's of the
	// wrong type". the latter happens when called by h_alloc to
	// check if e.g. a Tex object already exists; at that time,
	// only the corresponding VFile exists.
	Handle ret = -1;

	std::pair<It, It> range = key2idx->equal_range(key);
	for(It it = range.first; it != range.second; ++it)
	{
		ssize_t idx = it->second;
		HDATA* hd;
		if(h_data_from_idx(idx, hd) != INFO::OK)
			continue;
		if(hd->type != type || hd->key != key)
			continue;

		// found a match
		if(remove_option == KEY_REMOVE)
			key2idx->erase(it);
		ret = hd->h;
		break;
	}

	key2idx_wrapper.lock();
	return ret;
}


static void key_add(uintptr_t key, Handle h)
{
	Key2Idx* key2idx = key2idx_wrapper.get();
	if(!key2idx)
		return;

	const ssize_t idx = h_idx(h);
	// note: MSDN documentation of stdext::hash_multimap is incorrect;
	// there is no overload of insert() that returns pair<iterator, bool>.
	(void)key2idx->insert(std::make_pair(key, idx));

	key2idx_wrapper.lock();
}


static void key_remove(uintptr_t key, H_Type type)
{
	Handle ret = key_find(key, type, KEY_REMOVE);
	ENSURE(ret > 0);
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
	Status err = vtbl->validate(hd->user);
	ENSURE(err == INFO::OK);

	// make sure empty space in control block isn't touched
	// .. but only if we're not storing a filename there
	const u8* start = hd->user + vtbl->user_size;
	const u8* end   = hd->user + HDATA_USER_SIZE;
	for(const u8* p = start; p < end; p++)
		ENSURE(*p == 0);	// else: handle user data was overrun!
#else
	UNUSED2(hd);
#endif
}


static Status type_validate(H_Type type)
{
	if(!type)
		WARN_RETURN(ERR::INVALID_PARAM);
	if(type->user_size > HDATA_USER_SIZE)
		WARN_RETURN(ERR::LIMIT);
	if(type->name == 0)
		WARN_RETURN(ERR::INVALID_PARAM);

	return INFO::OK;
}


static Tag gen_tag()
{
	static Tag tag;
	tag += (1ull << IDX_BITS);
	// it's not easy to detect overflow, because compilers
	// are allowed to assume it'll never happen. however,
	// pow(2, 64-IDX_BITS) is "enough" anyway.
	return tag;
}


static Handle reuse_existing_handle(uintptr_t key, H_Type type, size_t flags)
{
	if(flags & RES_NO_CACHE)
		return 0;

	// object of specified key and type doesn't exist yet
	Handle h = h_find(type, key);
	if(h <= 0)
		return 0;

	HDATA* hd;
	RETURN_STATUS_IF_ERR(h_data_tag_type(h, type, hd));	// h_find means this won't fail

	hd->refs += 1;

	// we are reactivating a closed but cached handle.
	// need to generate a new tag so that copies of the
	// previous handle can no longer access the resource.
	// (we don't need to reset the tag in h_free, because
	// use before this fails due to refs > 0 check in h_user_data).
	if(hd->refs == 1)
	{
		const Tag tag = gen_tag();
		h = handle(h_idx(h), tag);	// can't fail
		hd->h = h;
	}

	return h;
}


static Status call_init_and_reload(Handle h, H_Type type, HDATA* hd, const PIVFS& vfs, const VfsPath& pathname, va_list* init_args)
{
	Status err = INFO::OK;
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
			err = vtbl->reload(hd->user, vfs, pathname, h);
			if(err == INFO::OK)
				warn_if_invalid(hd);
		}
		catch(std::bad_alloc&)
		{
			err  = ERR::NO_MEM;
		}
	}

	return err;
}


static Handle alloc_new_handle(H_Type type, const PIVFS& vfs, const VfsPath& pathname, uintptr_t key, size_t flags, va_list* init_args)
{
	HDATA* hd = (HDATA*)pool_alloc(&hpool, 0);
	if(!hd)
		WARN_RETURN(ERR::NO_MEM);
	new(&hd->pathname) VfsPath;

	ssize_t idx = h_idx_from_data(hd);
	RETURN_STATUS_IF_ERR(idx);

	// (don't want to do this before the add-reference exit,
	// so as not to waste tags for often allocated handles.)
	const Tag tag = gen_tag();
	Handle h = handle(idx, tag);	// can't fail.

	hd->h = h;
	hd->key  = key;
	hd->type = type;
	hd->refs = 1;
	if(!(flags & RES_NO_CACHE))
		hd->keep_open = 1;
	if(flags & RES_DISALLOW_RELOAD)
		hd->disallow_reload = 1;
	hd->unique = (flags & RES_UNIQUE) != 0;
	hd->pathname = pathname;

	if(key && !hd->unique)
		key_add(key, h);

	Status err = call_init_and_reload(h, type, hd, vfs, pathname, init_args);
	if(err < 0)
		goto fail;

	return h;

fail:
	// reload failed; free the handle
	hd->keep_open = 0;	// disallow caching (since contents are invalid)
	(void)h_free(h, type);	// (h_free already does WARN_IF_ERR)

	// note: since some uses will always fail (e.g. loading sounds if
	// g_Quickstart), do not complain here.
	return (Handle)err;
}


static std::recursive_mutex h_mutex;

// any further params are passed to type's init routine
Handle h_alloc(H_Type type, const PIVFS& vfs, const VfsPath& pathname, size_t flags, ...)
{
	std::lock_guard<std::recursive_mutex> lock(h_mutex);

	RETURN_STATUS_IF_ERR(type_validate(type));

	const uintptr_t key = fnv_hash(pathname.string().c_str(), pathname.string().length()*sizeof(pathname.string()[0]));

	// see if we can reuse an existing handle
	Handle h = reuse_existing_handle(key, type, flags);
	RETURN_STATUS_IF_ERR(h);
	// .. successfully reused the handle; refcount increased
	if(h > 0)
		return h;
	// .. need to allocate a new one:
	va_list args;
	va_start(args, flags);
	h = alloc_new_handle(type, vfs, pathname, key, flags, &args);
	va_end(args);
	return h;	// alloc_new_handle already does WARN_RETURN_STATUS_IF_ERR
}


//-----------------------------------------------------------------------------

static void h_free_hd(HDATA* hd)
{
	if(hd->refs > 0)
		hd->refs--;

	// still references open or caching requests it stays - do not release.
	if(hd->refs > 0 || hd->keep_open)
		return;

	// actually release the resource (call dtor, free control block).

	// h_alloc makes sure type != 0; if we get here, it still is
	H_VTbl* vtbl = hd->type;

	// call its destructor
	// note: H_TYPE_DEFINE currently always defines a dtor, but play it safe
	if(vtbl->dtor)
		vtbl->dtor(hd->user);

	if(hd->key && !hd->unique)
		key_remove(hd->key, hd->type);

#ifndef NDEBUG
	// to_string is slow for some handles, so avoid calling it if unnecessary
	if(debug_filter_allows("H_MGR|"))
	{
		wchar_t buf[H_STRING_LEN];
		if(vtbl->to_string(hd->user, buf) < 0)
			wcscpy_s(buf, ARRAY_SIZE(buf), L"(error)");
		debug_printf("H_MGR| free %s %s accesses=%lu %s\n", utf8_from_wstring(hd->type->name).c_str(), hd->pathname.string8().c_str(), (unsigned long)hd->num_derefs, utf8_from_wstring(buf).c_str());
	}
#endif

	hd->pathname.~VfsPath();	// FIXME: ugly hack, but necessary to reclaim memory
	memset(hd, 0, sizeof(*hd));
	pool_free(&hpool, hd);
}


Status h_free(Handle& h, H_Type type)
{
	std::lock_guard<std::recursive_mutex> lock(h_mutex);

	// 0-initialized or an error code; don't complain because this
	// happens often and is harmless.
	if(h <= 0)
		return INFO::OK;

	// wipe out the handle to prevent reuse but keep a copy for below.
	const Handle h_copy = h;
	h = 0;

	HDATA* hd;
	RETURN_STATUS_IF_ERR(h_data_tag_type(h_copy, type, hd));

	h_free_hd(hd);
	return INFO::OK;
}


//----------------------------------------------------------------------------
// remaining API

void* h_user_data(const Handle h, const H_Type type)
{
	HDATA* hd;
	if(h_data_tag_type(h, type, hd) != INFO::OK)
		return 0;

	if(!hd->refs)
	{
		// note: resetting the tag is not enough (user might pass in its value)
		DEBUG_WARN_ERR(ERR::LOGIC);	// no references to resource (it's cached, but someone is accessing it directly)
		return 0;
	}

	warn_if_invalid(hd);
	return hd->user;
}


VfsPath h_filename(const Handle h)
{
	// don't require type check: should be usable for any handle,
	// even if the caller doesn't know its type.
	HDATA* hd;
	if(h_data_tag(h, hd) != INFO::OK)
		return VfsPath();
	return hd->pathname;
}


// TODO: what if iterating through all handles is too slow?
Status h_reload(const PIVFS& vfs, const VfsPath& pathname)
{
	std::lock_guard<std::recursive_mutex> lock(h_mutex);

	const u32 key = fnv_hash(pathname.string().c_str(), pathname.string().length()*sizeof(pathname.string()[0]));

	// destroy (note: not free!) all handles backed by this file.
	// do this before reloading any of them, because we don't specify reload
	// order (the parent resource may be reloaded first, and load the child,
	// whose original data would leak).
	for(HDATA* hd = (HDATA*)hpool.da.base; hd < (HDATA*)(hpool.da.base + hpool.da.pos); hd = (HDATA*)(uintptr_t(hd)+hpool.el_size))
	{
		if(hd->key == 0 || hd->key != key || hd->disallow_reload)
			continue;
		hd->type->dtor(hd->user);
	}

	Status ret = INFO::OK;

	// now reload all affected handles
	size_t i = 0;
	for(HDATA* hd = (HDATA*)hpool.da.base; hd < (HDATA*)(hpool.da.base + hpool.da.pos); hd = (HDATA*)(uintptr_t(hd)+hpool.el_size), i++)
	{
		if(hd->key == 0 || hd->key != key || hd->disallow_reload)
			continue;

		Status err = hd->type->reload(hd->user, vfs, hd->pathname, hd->h);
		// don't stop if an error is encountered - try to reload them all.
		if(err < 0)
		{
			h_free(hd->h, hd->type);
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
	std::lock_guard<std::recursive_mutex> lock(h_mutex);
	return key_find(key, type);
}



// force the resource to be freed immediately, even if cached.
// tag is not checked - this allows the first Handle returned
// (whose tag will change after being 'freed', but remaining in memory)
// to later close the object.
// this is used when reinitializing the sound engine -
// at that point, all (cached) OpenAL resources must be freed.
Status h_force_free(Handle h, H_Type type)
{
	std::lock_guard<std::recursive_mutex> lock(h_mutex);

	// require valid index; ignore tag; type checked below.
	HDATA* hd;
	RETURN_STATUS_IF_ERR(h_data_no_tag(h, hd));
	if(hd->type != type)
		WARN_RETURN(ERR::H_TYPE_MISMATCH);
	hd->keep_open = 0;
	hd->refs = 0;
	h_free_hd(hd);
	return INFO::OK;
}


// increment Handle <h>'s reference count.
// only meant to be used for objects that free a Handle in their dtor,
// so that they are copy-equivalent and can be stored in a STL container.
// do not use this to implement refcounting on top of the Handle scheme,
// e.g. loading a Handle once and then passing it around. instead, have each
// user load the resource; refcounting is done under the hood.
void h_add_ref(Handle h)
{
	HDATA* hd;
	if(h_data_tag(h, hd) != INFO::OK)
		return;

	ENSURE(hd->refs);	// if there are no refs, how did the caller manage to keep a Handle?!
	hd->refs++;
}


// retrieve the internal reference count or a negative error code.
// background: since h_alloc has no way of indicating whether it
// allocated a new handle or reused an existing one, counting references
// within resource control blocks is impossible. since that is sometimes
// necessary (always wrapping objects in Handles is excessive), we
// provide access to the internal reference count.
intptr_t h_get_refcnt(Handle h)
{
	HDATA* hd;
	RETURN_STATUS_IF_ERR(h_data_tag(h, hd));

	ENSURE(hd->refs);	// if there are no refs, how did the caller manage to keep a Handle?!
	return hd->refs;
}


static ModuleInitState initState;

static Status Init()
{
	RETURN_STATUS_IF_ERR(pool_create(&hpool, hdata_cap*sizeof(HDATA), sizeof(HDATA)));
	return INFO::OK;
}

static void Shutdown()
{
	debug_printf("H_MGR| shutdown. any handle frees after this are leaks!\n");
	// objects that store handles to other objects are destroyed before their
	// children, so the subsequent forced destruction of the child here will
	// raise a double-free warning unless we ignore it. (#860, #915, #920)
	ignoreDoubleFree = true;

	std::lock_guard<std::recursive_mutex> lock(h_mutex);

	// forcibly close all open handles
	for(HDATA* hd = (HDATA*)hpool.da.base; hd < (HDATA*)(hpool.da.base + hpool.da.pos); hd = (HDATA*)(uintptr_t(hd)+hpool.el_size))
	{
		// it's already been freed; don't free again so that this
		// doesn't look like an error.
		if(hd->key == 0)
			continue;

		// disable caching; we need to release the resource now.
		hd->keep_open = 0;
		hd->refs = 0;

		h_free_hd(hd);
	}

	pool_destroy(&hpool);
}

void h_mgr_free_type(const H_Type type)
{
	ignoreDoubleFree = true;

	std::lock_guard<std::recursive_mutex> lock(h_mutex);

	// forcibly close all open handles of the specified type
	for(HDATA* hd = (HDATA*)hpool.da.base; hd < (HDATA*)(hpool.da.base + hpool.da.pos); hd = (HDATA*)(uintptr_t(hd)+hpool.el_size))
	{
		// free if not previously freed and only free the proper type
		if (hd->key == 0 || hd->type != type)
			continue;

		// disable caching; we need to release the resource now.
		hd->keep_open = 0;
		hd->refs = 0;

		h_free_hd(hd);
	}
}

void h_mgr_init()
{
	ModuleInit(&initState, Init);
}

void h_mgr_shutdown()
{
	ModuleShutdown(&initState, Shutdown);
}
