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
#include "mem.h"
#include "vfs.h"

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
	// cannot fail - it was successfully allocated

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

	// free its pointer
	switch(hd->ptype)
	{
	case PT_MEM:
		mem_free(hd->p);
		break;

	case PT_MAP:
		munmap(hd->p, hd->size);
		break;
	}

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


Handle h_find(const void* p, uint type, HDATA** puser)
{
	for(int idx = 0; idx <= last_in_use; idx++)
	{
		HDATA* hd = h_data(idx);	// guaranteed valid

		if(hd->p == p && hd->type == type)
		{
			if(puser)
				*puser = hd/*->user*/;
			return handle(idx);
		}
	}

	return 0;
}


Handle h_find(const u32 key, uint type, HDATA** puser)
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
		*puser = hd/*->user*/;
	return h;
}


int h_assign(Handle h, u8* p, size_t size, bool mem)
{
	HDATA* hd = h_data(h);
	if(!hd)
		return -1;

	if(hd->p || hd->size || hd->ptype != PT_NONE)
	{
		assert(!"h_assign: field(s) already assigned");
		return -1;
	}

	hd->p = p;
	hd->size = size;
	hd->ptype = mem? PT_MEM : PT_MAP;

	return 0;
}


Handle h_alloc(const u32 key, const uint type, /*const size_t user_size,*/ DTOR dtor, HDATA** puser)
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
		Handle h = h_find(key, type, &hd);
		if(h)
		{
			// only way to decide whether this handle is new, or a reference
			assert(hd->size != 0);

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
		*puser = hd/*->user*/;
	return handle(idx);
}


int h_free(const Handle h, const uint type)
{
	int idx = h_idx(h, type);
	if(idx >= 0)
		return h_free(idx);
	return -1;
}



HDATA* h_data(const Handle h, const uint type)
{
	int idx = h_idx(h, type);
	if(idx >= 0)
		return h_data(idx);
	return 0;
}



Handle res_load(const char* fn, uint type, DTOR dtor, void*& p, size_t& size, HDATA*& user)
{
	p = 0;
	size = 0;
	user = 0;

	u32 fn_hash = fnv_hash(fn, strlen(fn));
		// TODO: fn is usually a constant; pass fn len if too slow

	HDATA* hd;
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
user=hd;
	return h;
}




