// handle-based resource manager 
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

#include "types.h"
#include "zip.h"
#include "posix.h"
#include "misc.h"
#include "res.h"
#include "mem.h"
#include "vfs.h"


// handle (32 bits)
// .. make sure this is the same handle we opened
const uint HTAG_BITS = 16;
// .. index into array => = log2(max open handles)
const uint HIDX_BITS = 12;

// together, <= 32-TAG_BITS bits
const uint HTYPE_BITS = 4;
const uint HREF_BITS  = 8;

const int HDATA_USER_SIZE = 20;

// 32 bytes
struct HDATA
{
	u32 key;
	u32 tag   : HTAG_BITS;
	u32 type  : HTYPE_BITS;		// handle's type (e.g. texture, sound)
	u32 refs  : HREF_BITS;

	u32 unused;	// TODO: type pointer?

	u8 user[HDATA_USER_SIZE];
};

static const ulong hdata_cap = 1ul << HIDX_BITS;
static const uint type_cap = 1ul << HTYPE_BITS;

// array of pages for better locality, less fragmentation
static const uint PAGE_SIZE = 4096;
static const uint hdata_per_page = PAGE_SIZE / sizeof(HDATA);
static const uint num_pages = hdata_cap / hdata_per_page;
static HDATA* pages[num_pages];

static int first_free = -1;		// don't want to scan array every h_alloc
static int last_in_use = -1;	// don't search unused entries


static void(*dtors[type_cap])(void*);


// get pointer to handle data (non-contiguous array)
static HDATA* h_data(const int idx)
{
	if(idx > hdata_cap)
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


// get array index from handle
static int h_idx(const Handle h, const uint type)
{
	const int idx = h & ((1 << HIDX_BITS)-1);
	if(idx > last_in_use)
		return -1;

	const HDATA* hd = h_data(idx);
	// cannot fail - all HDATA up to last_in_use are valid

	const u32 tag = h >> HIDX_BITS;
	// note: tag = 0 marks unused entries => is invalid
	if(!tag || hd->tag != tag || hd->type != type)
		return -1;

	return idx;
}


static Handle handle(const int idx)
{
	const HDATA* hd = h_data(idx);
	if(!hd)	// out of memory
		return 0;
	return (hd->tag) << HIDX_BITS | (u32)idx;
}



static int h_free(const int idx)
{
	HDATA* hd = h_data(idx);
	if(!hd)
		return -1;

	// not the last reference
	if(--hd->refs)
		return 0;

	// TODO: keep this handle open (cache)

	// call its type's destructor
	if(dtors[hd->type])
		dtors[hd->type](hd);

	memset(hd, 0, sizeof(HDATA));

	if(first_free == -1 || idx < first_free)
		first_free = idx;

	return 0;
}


static void cleanup(void)
{
	int i;

	// close open handles
	for(i = 0; i < last_in_use; i++)
		h_free(i);

	// free hdata array
	for(i = 0; i < (int)num_pages; i++)
	{
		free(pages[i]);
		pages[i] = 0;
	}
}


Handle h_find(const u32 key, uint type, void** puser)
{
	int idx;
	HDATA* hd;

	// already have first free entry cached - just search
	if(first_free != -1)
	{
		for(idx = 0; idx <= last_in_use; idx++)
		{
			hd = h_data(idx);	// guaranteed valid
			if(hd->key == key && hd->type == type)
				goto found;
		}
	}
	// search and remember first free entry (slower)
	else
	{
		for(idx = 0; idx <= last_in_use; idx++)
		{
			hd = h_data(idx);	// guaranteed valid
			if(!hd->tag && first_free == -1)
				first_free = idx;
			else if(hd->key == key && hd->type == type)
				goto found;
		}
	}

	// not found
	return 0;

found:
	Handle h = handle(idx);
	if(puser)
		*puser = hd->user;
	return h;
}


Handle h_alloc(const u32 key, const uint type, /*const size_t user_size,*/ H_DTOR dtor, void** puser)
{
	ONCE(atexit(cleanup))
/*
	if(user_size > HDATA_USER_SIZE)
	{
		assert(!"h_alloc: not enough space in entry for user data");
		return 0;
	}
*/
	if(type >= type_cap)
	{
		assert(!"h_alloc: invalid type");
		return 0;
	}

	if(dtor)
	{
		// registering a second dtor for type
		if(dtors[type] && dtors[type] != dtor)
		{
			assert(!"h_alloc: registering a second, different dtor for type");
			return 0;
		}
		dtors[type] = dtor;
	}

	int idx;
	HDATA* hd;

	if(key)
	{
		// object already loaded?
		Handle h = h_find(key, type, 0);
		if(h)
		{
			hd = h_data(h_idx(h, type));

			if(hd->refs == (1ul << HREF_BITS))
			{
				assert(!"h_alloc: too many references to a handle - increase REF_BITS");
				return 0;
			}
			hd->refs++;

			if(puser)
				*puser = hd;

			return h;
		}
	}

	// cached
	if(first_free != -1)
	{
		idx = first_free;
		hd = h_data(idx);
	}
	// search handle data for first free entry
	else
		for(idx = 0; idx < hdata_cap; idx++)
		{
			hd = h_data(idx);
			// not enough memory - abort (don't leave a hole in the array)
			if(!hd)
				return 0;
			// found an empty entry - done
			if(!hd->tag)
				break;
		}

	if(idx >= hdata_cap)
	{
		assert(!"h_alloc: too many open handles (increase IDX_BITS)");
		return 0;
	}

	// check if next entry is free
	HDATA* hd2 = h_data(idx+1);
	if(hd2 && hd2->tag == 0)
		first_free = idx+1;
	else
		first_free = -1;

	if(idx > last_in_use)
		last_in_use = idx;

	static u32 tag;
	if(++tag >= (1 << HTAG_BITS))
	{
		assert(!"h_alloc: tag overflow - may not notice stale handle reuse (increase TAG_BITS)");
		tag = 1;
	}

	hd->key = key;
	hd->tag = tag;
	hd->type = type;

	if(puser)
		*puser = hd->user;
	return handle(idx);
}


int h_free(Handle& h, const uint type)
{
	int idx = h_idx(h, type);
	h = 0;
	if(idx >= 0)
		return h_free(idx);
	return -1;
}



void* h_user_data(const Handle h, const uint type)
{
	int idx = h_idx(h, type);
	if(idx >= 0)
		return h_data(idx)->user;	// pointer is always valid if index is in range
	return 0;
}




