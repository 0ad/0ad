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
 * dynamic (expandable) array
 */

#ifndef INCLUDED_ALLOCATORS_DYNARRAY
#define INCLUDED_ALLOCATORS_DYNARRAY

#include "lib/posix/posix_mman.h"	// PROT_*

/**
 * provides a memory range that can be expanded but doesn't waste
 * physical memory or relocate itself.
 *
 * works by preallocating address space and committing as needed.
 * used as a building block for other allocators.
 **/
struct DynArray
{
	u8* base;
	size_t max_size_pa;	 /// reserved
	size_t cur_size;	 /// committed
	size_t cur_size_pa;

	size_t pos;
};


/**
 * ready the DynArray object for use.
 *
 * no virtual memory is actually committed until calls to da_set_size.
 *
 * @param da DynArray.
 * @param max_size size [bytes] of address space to reserve (*);
 * the DynArray can never expand beyond this.
 * (* rounded up to next page size multiple)
 * @return Status.
 **/
LIB_API Status da_alloc(DynArray* da, size_t max_size);

/**
 * free all memory (address space + physical) that constitutes the
 * given array.
 *
 * use-after-free is impossible because the memory is unmapped.
 *
 * @param da DynArray* zeroed afterwards.
 * @return Status
 **/
LIB_API Status da_free(DynArray* da);

/**
 * expand or shrink the array: changes the amount of currently committed
 * (i.e. usable) memory pages.
 *
 * @param da DynArray.
 * @param new_size target size (rounded up to next page multiple).
 * pages are added/removed until this is met.
 * @return Status.
 **/
LIB_API Status da_set_size(DynArray* da, size_t new_size);

/**
 * Make sure at least \<size\> bytes starting at da->pos are committed and
 * ready for use.
 *
 * @param da DynArray*
 * @param size Minimum amount to guarantee [bytes]
 * @return Status
 **/
LIB_API Status da_reserve(DynArray* da, size_t size);

/**
 * "write" to array, i.e. copy from the given buffer.
 *
 * starts at offset DynArray.pos and advances this.
 *
 * @param da DynArray.
 * @param data_src source memory
 * @param size [bytes] to copy
 * @return Status.
 **/
LIB_API Status da_append(DynArray* da, const void* data_src, size_t size);

#endif	// #ifndef INCLUDED_ALLOCATORS_DYNARRAY
