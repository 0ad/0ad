/* Copyright (C) 2011 Wildfire Games.
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
 * virtual memory interface. supercedes POSIX mmap; provides support for
 * large pages, autocommit, and specifying protection flags during allocation.
 */

#ifndef INCLUDED_SYSDEP_VM
#define INCLUDED_SYSDEP_VM

#include "lib/posix/posix_mman.h"	// PROT_*

namespace vm {

// committing large pages (2 MiB) instead of regular 4 KiB pages can
// increase TLB coverage and reduce misses for sequential access patterns.
// however, small page TLBs have more entries, making them better suited
// to random accesses. it may also take a long time to find/free up
// contiguous regions of physical memory for large pages. applications
// can express their preference or go along with the default,
// which depends on several factors such as allocation size.
enum PageType
{
	kLarge,     // use large if available
	kSmall,     // always use small
	kDefault	// heuristic
};

/**
 * reserve address space and set the parameters for any later
 * on-demand commits.
 *
 * @param size desired number of bytes. any additional space
 *   in the last page is also accessible.
 * @param commitSize [bytes] how much to commit each time.
 *   larger values reduce the number of page faults at the cost of
 *   additional internal fragmentation. must be a multiple of
 *   largePageSize unless pageType == kSmall.
 * @param pageType chooses between large/small pages for commits.
 * @param prot memory protection flags for newly committed pages.
 * @return base address (aligned to the respective page size) or
 *   0 if address space/descriptor storage is exhausted
 *   (an error dialog will also be raised).
 *   must be freed via ReleaseAddressSpace.
**/
LIB_API void* ReserveAddressSpace(size_t size, size_t commitSize = g_LargePageSize, PageType pageType = kDefault, int prot = PROT_READ|PROT_WRITE);

/**
 * release address space and decommit any memory.
 *
 * @param p a pointer previously returned by ReserveAddressSpace.
 * @param size is required by the POSIX implementation and
 *   ignored on Windows. it also ensures compatibility with UniqueRange.
 **/
LIB_API void ReleaseAddressSpace(void* p, size_t size = 0);


/**
 * map physical memory to previously reserved address space.
 *
 * @param address, size need not be aligned, but this function commits
 *   any pages intersecting that interval.
 * @param pageType, prot - see ReserveAddressSpace.
 * @return whether memory was successfully committed.
 *
 * note: committing only maps virtual pages and does not actually allocate
 * page frames. Windows XP uses a first-touch heuristic - the page will
 * be taken from the node whose processor caused the fault.
 * therefore, worker threads should be the first to write to their memory.
 *
 * (this is surprisingly slow in XP, possibly due to PFN lock contention)
 **/
LIB_API bool Commit(uintptr_t address, size_t size, PageType pageType = kDefault, int prot = PROT_READ|PROT_WRITE);

/**
 * unmap physical memory.
 *
 * @return whether the operation succeeded.
 **/
LIB_API bool Decommit(uintptr_t address, size_t size);


/**
 * set the memory protection flags for all pages that intersect
 * the given interval.
 * the pages must currently be committed.
 *
 * @param prot memory protection flags: PROT_NONE or a combination of
 *   PROT_READ, PROT_WRITE, PROT_EXEC.
 **/
LIB_API bool Protect(uintptr_t address, size_t size, int prot);


/**
 * reserve address space and commit memory.
 *
 * @param size [bytes] to allocate.
 * @param pageType, prot - see ReserveAddressSpace.
 * @return zero-initialized memory aligned to the respective
 *   page size.
 **/
LIB_API void* Allocate(size_t size, PageType pageType = kDefault, int prot = PROT_READ|PROT_WRITE);

/**
 * decommit memory and release address space.
 *
 * @param p a pointer previously returned by Allocate.
 * @param size is required by the POSIX implementation and
 *   ignored on Windows. it also ensures compatibility with UniqueRange.
 *
 * (this differs from ReleaseAddressSpace, which must account for
 * extra padding/alignment to largePageSize.)
 **/
LIB_API void Free(void* p, size_t size = 0);


/**
 * install a handler that attempts to commit memory whenever a
 * read/write page fault is encountered. thread-safe.
 **/
LIB_API void BeginOnDemandCommits();

/**
 * decrements the reference count begun by BeginOnDemandCommit and
 * removes the page fault handler when it reaches 0. thread-safe.
 **/
LIB_API void EndOnDemandCommits();


LIB_API void DumpStatistics();

}	// namespace vm

#endif	// #ifndef INCLUDED_SYSDEP_VM
