/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : pool.cpp
 * Project     : 0 A.D.
 * Description : pool allocator
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "pool.h"

#include "mem_util.h"


LibError pool_create(Pool* p, size_t max_size, size_t el_size)
{
	if(el_size == POOL_VARIABLE_ALLOCS)
		p->el_size = 0;
	else
		p->el_size = mem_RoundUpToAlignment(el_size);
	p->freelist = 0;
	RETURN_ERR(da_alloc(&p->da, max_size));
	return INFO::OK;
}


LibError pool_destroy(Pool* p)
{
	// don't be picky and complain if the freelist isn't empty;
	// we don't care since it's all part of the da anyway.
	// however, zero it to prevent further allocs from succeeding.
	p->freelist = 0;
	return da_free(&p->da);
}


bool pool_contains(const Pool* p, void* el)
{
	// outside of our range
	if(!(p->da.base <= el && el < p->da.base+p->da.pos))
		return false;
	// sanity check: it should be aligned (if pool has fixed-size elements)
	if(p->el_size)
		debug_assert((uintptr_t)((u8*)el - p->da.base) % p->el_size == 0);
	return true;
}


void* pool_alloc(Pool* p, size_t size)
{
	// if pool allows variable sizes, go with the size parameter,
	// otherwise the pool el_size setting.
	const size_t el_size = p->el_size? p->el_size : mem_RoundUpToAlignment(size);

	// note: this can never happen in pools with variable-sized elements
	// because they disallow pool_free.
	void* el = mem_freelist_Detach(p->freelist);
	if(el)
		goto have_el;

	// alloc a new entry
	{
		// expand, if necessary
		if(da_reserve(&p->da, el_size) < 0)
			return 0;

		el = p->da.base + p->da.pos;
		p->da.pos += el_size;
	}

have_el:
	debug_assert(pool_contains(p, el));	// paranoia
	return el;
}


void pool_free(Pool* p, void* el)
{
	// only allowed to free items if we were initialized with
	// fixed el_size. (this avoids having to pass el_size here and
	// check if requested_size matches that when allocating)
	if(p->el_size == 0)
	{
		debug_assert(0);	// cannot free variable-size items
		return;
	}

	if(pool_contains(p, el))
		mem_freelist_AddToFront(p->freelist, el);
	else
		debug_assert(0);	// invalid pointer (not in pool)
}


void pool_free_all(Pool* p)
{
	p->freelist = 0;

	// must be reset before da_set_size or CHECK_DA will complain.
	p->da.pos = 0;

	da_set_size(&p->da, 0);
}
