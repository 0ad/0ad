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
// .. detect use of already freed handles
static const uint TAG_BITS = 18;
// .. index into array => = log2(max open handles)
static const uint IDX_BITS = 14;

static const uint TYPE_BITS = 5;
static const uint REF_BITS  = 9;

struct Res
{
	u32 key;
	u32 tag : TAG_BITS;
	u32 type : TYPE_BITS;
	u32 refs : REF_BITS;
};

static const ulong res_array_cap = 1ul << IDX_BITS;
static const uint max_types = 1ul << TYPE_BITS;

// static allocation for simplicity; mem usage isn't significant
static Res res_array[res_array_cap];
static int first_free = -1;
static int max_idx = -1;	// index of last in-use entry

// array of pages for better locality, less fragmentation
static const uint PAGE_SIZE = 4096;
static const uint hdata_per_page = PAGE_SIZE / sizeof(HDATA);
static const uint num_pages = res_array_cap / hdata_per_page;
static HDATA* pages[num_pages];

static void(*dtors[max_types])(HDATA*);








static Handle handle(const uint idx)
{
	if(idx >= (1 << IDX_BITS))
	{
		assert(!"invalid index passed to handle()");
		return 0;
	}
	return (res_array[idx].tag << IDX_BITS) | idx;
}


static int h_idx(Handle h, uint type)
{
	int idx = h & ((1 << IDX_BITS)-1);
	const Res& r = res_array[idx];

	u32 tag = h >> IDX_BITS;
	if(!tag || r.tag != tag || r.type != type)
		return -1;

	return idx;
}


static void cleanup(void)
{
	int i;

	// close open handles
	for(i = 0; i < max_idx; i++)
		h_free(handle(i));

	// free internal data space
	for(i = 0; i < (int)num_pages; i++)
	{
		free(pages[i]);
		pages[i] = 0;
	}
}


Handle h_alloc(const u32 key, const uint type, DTOR dtor, HDATA*& hd)
{
	ONCE(atexit(cleanup))

	if(type >= max_types)
		return 0;

	if(dtor)
	{
		// registering a second dtor for type
		if(dtors[type] && dtors[type] != dtor)
			return 0;
		dtors[type] = dtor;
	}

	Handle h;
	int idx;
	Res* r = res_array;

	h = h_find(key, type, hd);
	if(h)
	{
		idx = h_idx(h, type);
		r = &res_array[idx];
		if(r->refs == 1ul << REF_BITS)
		{
			assert(!"too many references to a handle - increase REF_BITS");
			return 0;
		}
		r->refs++;
		return h;
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
	if(idx+1 < res_array_cap && !r[1].key)
		first_free = idx+1;
	else
		first_free = -1;

	if(idx >= res_array_cap)
	{
		assert(!"too many open handles (increase IDX_BITS)");
		return 0;
	}

	if(idx > max_idx)
		max_idx = idx;

	static u32 tag;
	if(++tag >= (1 << TAG_BITS))
	{
		assert(!"tag overflow - may not notice stale handle reuse (increase TAG_BITS)");
		tag = 1;
	}

	r->key = key;
	r->tag = tag;
	r->type = type;

	h = handle(idx);
	hd = h_data(h, type);
	return h;
}


Handle h_find(const u32 key, uint type, HDATA*& hd)
{
	int idx;
	const Res* r = res_array;

	// already have first free entry cached - just search
	if(first_free != -1)
	{
		for(idx = 0; idx <= max_idx; idx++, r++)
			if(r->key == key && r->type == type)
				goto found;
	}
	// search and remember first free entry (slower)
	else
	{
		for(idx = 0; idx <= max_idx; idx++, r++)
			if(!r->tag && first_free == -1)
				first_free = idx;
			else if(r->key == key && r->type == type)
				goto found;
	}

	// not found
	return 0;

found:
	Handle h = handle(idx);
	hd = h_data(h, type);
	return h;
}


int h_free(const Handle h, const uint type)
{
	int idx = h_idx(h, type);
	if(idx == -1)
		return -1;

	Res& r = res_array[idx];
	if(--r.refs)
		return 0;

	HDATA* hd = h_data(h, type);
	if(hd && dtors[type])
		dtors[type](hd);

	r.key = 0;
	r.tag = 0;

	if(first_free == -1 || idx < first_free)
		first_free = idx;

	return 0;
}


HDATA* h_data(const Handle h, const uint type)
{
	int idx = h_idx(h, type);
	if(idx == -1)
		return 0;

	HDATA*& page = pages[idx / hdata_per_page];
	if(!page)
		page = (HDATA*)malloc(PAGE_SIZE);
	if(!page)
		return 0;

	return &page[idx % hdata_per_page];
}



Handle res_load(const char* fn, uint type, DTOR dtor, void*& p, size_t& size, HDATA*& hd)
{
	p = 0;
	size = 0;
	hd = 0;

	u32 fn_hash = fnv_hash(fn, strlen(fn));
		// TODO: fn is usually a constant; pass fn len if too slow

	Handle h = h_alloc(fn_hash, type, dtor, hd);
	if(!h || !hd)
		return 0;

	Handle hf = vfs_open(fn);
	int err = vfs_read(hf, p, size, 0);
	vfs_close(hf);
	if(err < 0)
		return 0;

	hd->p = p;
	hd->size = size;

	return h;
}




