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
 * memory suballocators.
 */

#include "precompiled.h"
#include "allocators.h"

#include "lib/sysdep/cpu.h"	// cpu_CAS
#include "lib/bits.h"

#include "mem_util.h"


//-----------------------------------------------------------------------------
// page aligned allocator
//-----------------------------------------------------------------------------

void* page_aligned_alloc(size_t unaligned_size)
{
	const size_t size_pa = mem_RoundUpToPage(unaligned_size);
	u8* p = 0;
	RETURN0_IF_ERR(mem_Reserve(size_pa, &p));
	RETURN0_IF_ERR(mem_Commit(p, size_pa, PROT_READ|PROT_WRITE));
	return p;
}


void page_aligned_free(void* p, size_t unaligned_size)
{
	if(!p)
		return;
	debug_assert(mem_IsPageMultiple((uintptr_t)p));
	const size_t size_pa = mem_RoundUpToPage(unaligned_size);
	(void)mem_Release((u8*)p, size_pa);
}


//-----------------------------------------------------------------------------
// matrix allocator
//-----------------------------------------------------------------------------

void** matrix_alloc(size_t cols, size_t rows, size_t el_size)
{
	const size_t initial_align = 64;
	// note: no provision for padding rows. this is a bit more work and
	// if el_size isn't a power-of-2, performance is going to suck anyway.
	// otherwise, the initial alignment will take care of it.

	const size_t ptr_array_size = cols*sizeof(void*);
	const size_t row_size = cols*el_size;
	const size_t data_size = rows*row_size;
	const size_t total_size = ptr_array_size + initial_align + data_size;

	void* p = malloc(total_size);
	if(!p)
		return 0;

	uintptr_t data_addr = (uintptr_t)p + ptr_array_size + initial_align;
	data_addr -= data_addr % initial_align;

	// alignment check didn't set address to before allocation
	debug_assert(data_addr >= (uintptr_t)p+ptr_array_size);

	void** ptr_array = (void**)p;
	for(size_t i = 0; i < cols; i++)
	{
		ptr_array[i] = (void*)data_addr;
		data_addr += row_size;
	}

	// didn't overrun total allocation
	debug_assert(data_addr <= (uintptr_t)p+total_size);

	return ptr_array;
}


void matrix_free(void** matrix)
{
	free(matrix);
}


//-----------------------------------------------------------------------------
// allocator optimized for single instances
//-----------------------------------------------------------------------------

void* single_calloc(void* storage, volatile uintptr_t* in_use_flag, size_t size)
{
	// sanity check
	debug_assert(*in_use_flag == 0 || *in_use_flag == 1);

	void* p;

	// successfully reserved the single instance
	if(cpu_CAS(in_use_flag, 0, 1))
		p = storage;
	// already in use (rare) - allocate from heap
	else
		p = new u8[size];

	memset(p, 0, size);
	return p;
}


void single_free(void* storage, volatile uintptr_t* in_use_flag, void* p)
{
	// sanity check
	debug_assert(*in_use_flag == 0 || *in_use_flag == 1);

	if(p == storage)
	{
		if(cpu_CAS(in_use_flag, 1, 0))
		{
			// ok, flag has been reset to 0
		}
		else
			debug_assert(0);	// in_use_flag out of sync (double free?)
	}
	// was allocated from heap
	else
	{
		// single instance may have been freed by now - cannot assume
		// anything about in_use_flag.

		delete[] (u8*)p;
	}
}


//-----------------------------------------------------------------------------
// static allocator
//-----------------------------------------------------------------------------

void* static_calloc(StaticStorage* ss, size_t size)
{
	void* p = (void*)round_up((uintptr_t)ss->pos, (uintptr_t)16u);
	ss->pos = (u8*)p+size;
	debug_assert(ss->pos <= ss->end);
	return p;
}
