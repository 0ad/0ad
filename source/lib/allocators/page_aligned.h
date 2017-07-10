/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_ALLOCATORS_PAGE_ALIGNED
#define INCLUDED_ALLOCATORS_PAGE_ALIGNED

#include "lib/posix/posix_mman.h"	// PROT_*

// very thin wrapper on top of sys/mman.h that makes the intent more obvious
// (its commit/decommit semantics are difficult to tell apart)
LIB_API Status mem_Reserve(size_t size, u8** pp);
LIB_API Status mem_Release(u8* p, size_t size);
LIB_API Status mem_Commit(u8* p, size_t size, int prot);
LIB_API Status mem_Decommit(u8* p, size_t size);
LIB_API Status mem_Protect(u8* p, size_t size, int prot);


/**
 * allocate memory aligned to the system page size.
 *
 * this is useful for file_cache_alloc, which uses this allocator to
 * get sector-aligned (hopefully; see sys_max_sector_size) IO buffers.
 *
 * note that this allocator is stateless and very little error checking
 * can be performed.
 *
 * the memory is initially writable and you can use mprotect to set other
 * access permissions if desired.
 *
 * @param unaligned_size minimum size [bytes] to allocate.
 * @return page-aligned and -padded memory or 0 on error / out of memory.
 **/
LIB_API void* page_aligned_alloc(size_t unaligned_size);

/**
 * free a previously allocated page-aligned region.
 *
 * @param p Exact value returned from page_aligned_alloc
 * @param unaligned_size Exact value passed to page_aligned_alloc
 **/
LIB_API void page_aligned_free(void* p, size_t unaligned_size);

#endif	// #ifndef INCLUDED_ALLOCATORS_PAGE_ALIGNED
