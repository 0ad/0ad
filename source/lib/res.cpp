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
#include "unzip.h"
#include "posix.h"
#include "misc.h"
#include "res.h"
#include "vfs.h"

// handle (32 bits)
// .. make sure this is the same handle we opened
static const uint TAG_BITS = 16;
// .. index into array => = log2(max open handles)
static const uint IDX_BITS = 12;

static const uint OWNER_BITS = 4;
static const uint TYPE_BITS = 4;
static const uint REF_BITS  = 8;

struct Res
{
	u32 key;
	u32 tag   : TAG_BITS;
	u32 type  : TYPE_BITS;
	u32 owner : OWNER_BITS;
	u32 refs  : REF_BITS;
};

static const ulong res_array_cap = 1ul << IDX_BITS;
static const uint owner_cap = 1ul << OWNER_BITS;

// static allocation for simplicity; mem usage isn't significant
static Res res_array[res_array_cap];
static int first_free = -1;
static int max_idx = -1;	// index of last in-use entry

// array of pages for better locality, less fragmentation
static const uint PAGE_SIZE = 4096;
static const uint hdata_per_page = PAGE_SIZE / sizeof(HDATA);
static const uint num_pages = res_array_cap / hdata_per_page;
static HDATA* pages[num_pages];

static void(*dtors[owner_cap])(HDATA*);



static Handle handle(const int idx)
{
	const Res& r = res_array[idx];
	return (r.tag) << IDX_BITS | (u32)idx;
}


static int h_idx(const Handle h, const uint owner)
{
	int idx = h & ((1 << IDX_BITS)-1);
	const Res& r = res_array[idx];

	u32 tag = h >> IDX_BITS;
	if(!tag || r.tag != tag || r.owner != owner)
		return -1;

	return idx;
}


static HDATA* h_data(const int idx)
{
	HDATA*& page = pages[idx / hdata_per_page];
	if(!page)
	{
		page = (HDATA*)calloc(PAGE_SIZE, 1);
		if(!page)
			return 0;
	}

	return &page[idx % hdata_per_page];
}


static int h_free(const int idx)
{
	Res& r = res_array[idx];
	if(--r.refs)
		return 0;

	HDATA* hd = h_data(idx);
	if(hd)
	{
		if(dtors[r.owner])
			dtors[r.owner](hd);
		memset(hd, 0, sizeof(HDATA));
	}

	memset(&r, 0, sizeof(r));

	if(first_free == -1 || idx < first_free)
		first_free = idx;

	return 0;
}


static void cleanup(void)
{
	int i;

	// close open handles
	for(i = 0; i < max_idx; i++)
		h_free(i);

	// free internal data space
	for(i = 0; i < (int)num_pages; i++)
	{
		free(pages[i]);
		pages[i] = 0;
	}
}


Handle h_find(const void* p, uint owner, HDATA** phd)
{
	const Res* r = res_array;

	for(int idx = 0; idx <= max_idx; idx++, r++)
	{
		if(r->owner != owner)
			continue;

		HDATA* hd = h_data(idx);
		if(hd && hd->p == p)
		{
			if(phd)
				*phd = hd;
			return handle(idx);
		}
	}

	return 0;
}


Handle h_find(const u32 key, uint owner, HDATA** phd)
{
	int idx;
	const Res* r = res_array;

	// already have first free entry cached - just search
	if(first_free != -1)
	{
		for(idx = 0; idx <= max_idx; idx++, r++)
			if(r->key == key && r->owner == owner)
				goto found;
	}
	// search and remember first free entry (slower)
	else
	{
		for(idx = 0; idx <= max_idx; idx++, r++)
			if(!r->tag && first_free == -1)
				first_free = idx;
			else if(r->key == key && r->owner == owner)
				goto found;
	}

	// not found
	return 0;

found:
	Handle h = handle(idx);
	if(phd)
	{
		HDATA* hd = h_data(h, owner);
		if(!hd)
			return 0;
		*phd = hd;
	}
	return h;
}


Handle h_alloc(const u32 key, const uint owner, DTOR dtor, HDATA** phd)
{
	ONCE(atexit(cleanup))

	if(owner >= owner_cap)
		return 0;

	if(dtor)
	{
		// registering a second dtor for owner
//		if(dtors[owner] && dtors[owner] != dtor)
//			return 0;
		dtors[owner] = dtor;
	}

	int idx;
	Res* r = res_array;

	HDATA* hd;
	Handle h;
	if(key)
	{
		h = h_find(key, owner, &hd);
		if(h)
		{
			idx = h_idx(h, owner);
			r = &res_array[idx];

			assert(hd->size != 0);

			if(r->refs == (1ul << REF_BITS))
			{
				assert(!"h_alloc: too many references to a handle - increase REF_BITS");
				return 0;
			}
			r->refs++;

			return h;
		}
	}

	// cached
	if(first_free != -1)
	{
		idx = first_free;
		r = &res_array[idx];
	}
	// search res_array for first entry
	else
		for(idx = 0; idx < res_array_cap; idx++, r++)
			if(!r->tag)
				break;

	// check if next entry is free
	if(idx+1 < res_array_cap && !(r+1)->key)
		first_free = idx+1;
	else
		first_free = -1;

	if(idx >= res_array_cap)
	{
		assert(!"h_alloc: too many open handles (increase IDX_BITS)");
		return 0;
	}

	if(idx > max_idx)
		max_idx = idx;

	static u32 tag;
	if(++tag >= (1 << TAG_BITS))
	{
		assert(!"h_alloc: tag overflow - may not notice stale handle reuse (increase TAG_BITS)");
		tag = 1;
	}

	r->key = key;
	r->tag = tag;
	r->owner = owner;

	h = handle(idx);

	// caller wants HDATA* returned
	if(phd)
	{
		HDATA* hd = h_data(h, owner);
		// not enough mem - fail
		if(!hd)
		{
			h_free(h, owner);
			return 0;
		}
		*phd = hd;
	}

	return h;
}


int h_free(const Handle h, const uint owner)
{
	int idx = h_idx(h, owner);
	if(idx >= 0)
		return h_free(idx);
	return -1;
}



HDATA* h_data(const Handle h, const uint owner)
{
	int idx = h_idx(h, owner);
	if(idx >= 0)
		return h_data(idx);
	return 0;
}



Handle res_load(const char* fn, uint type, DTOR dtor, void*& p, size_t& size, HDATA*& hd)
{
	p = 0;
	size = 0;
	hd = 0;

	u32 fn_hash = fnv_hash(fn, strlen(fn));
		// TODO: fn is usually a constant; pass fn len if too slow

	Handle h = h_alloc(fn_hash, type, dtor, &hd);
	if(!h)
		return 0;

	// already open - return a reference
	if(hd->size)
		return h;

	Handle hf = vfs_open(fn);
	int err = vfs_read(hf, p, size, 0);
	vfs_close(hf);
	if(err < 0)
		return 0;

	if(!p)
	{
		h_free(h, type);
		return 0;
	}

	hd->p = p;
	hd->size = size;

	return h;
}




