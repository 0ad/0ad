/* Copyright (c) 2010 Wildfire Games
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
 * pool allocator
 */

#include "precompiled.h"
#include "lib/allocators/pool.h"

#include "lib/alignment.h"
#include "lib/allocators/freelist.h"

#include "lib/timer.h"

TIMER_ADD_CLIENT(tc_pool_alloc);


LibError pool_create(Pool* p, size_t max_size, size_t el_size)
{
	if(el_size == POOL_VARIABLE_ALLOCS)
		p->el_size = 0;
	else
		p->el_size = Align<allocationAlignment>(el_size);
	p->freelist = mem_freelist_Sentinel();
	RETURN_ERR(da_alloc(&p->da, max_size));
	return INFO::OK;
}


LibError pool_destroy(Pool* p)
{
	// don't be picky and complain if the freelist isn't empty;
	// we don't care since it's all part of the da anyway.
	// however, zero it to prevent further allocs from succeeding.
	p->freelist = mem_freelist_Sentinel();
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
	TIMER_ACCRUE(tc_pool_alloc);
	// if pool allows variable sizes, go with the size parameter,
	// otherwise the pool el_size setting.
	const size_t el_size = p->el_size? p->el_size : Align<allocationAlignment>(size);

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
	p->freelist = mem_freelist_Sentinel();

	// must be reset before da_set_size or CHECK_DA will complain.
	p->da.pos = 0;

	da_set_size(&p->da, 0);
}

size_t pool_committed(Pool* p)
{
	return p->da.cur_size;
}
