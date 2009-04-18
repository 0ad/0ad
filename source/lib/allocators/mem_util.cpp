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
 * File        : mem_util.cpp
 * Project     : 0 A.D.
 * Description : memory allocator helper routines.
 * =========================================================================
 */

#include "precompiled.h"
#include "mem_util.h"

#include "lib/bits.h"				// round_up
#include "lib/posix/posix_mman.h"
#include "lib/sysdep/os_cpu.h"			// os_cpu_PageSize


bool mem_IsPageMultiple(uintptr_t x)
{
	return (x & (os_cpu_PageSize()-1)) == 0;
}

size_t mem_RoundUpToPage(size_t size)
{
	return round_up(size, os_cpu_PageSize());
}

size_t mem_RoundUpToAlignment(size_t size)
{
	// all allocators should align to at least this many bytes:
	const size_t alignment = 8;
	return round_up(size, alignment);
}


//-----------------------------------------------------------------------------

static inline LibError LibError_from_mmap(void* ret, bool warn_if_failed = true)
{
	if(ret != MAP_FAILED)
		return INFO::OK;
	return LibError_from_errno(warn_if_failed);
}

// "anonymous" effectively means mapping /dev/zero, but is more efficient.
// MAP_ANONYMOUS is not in SUSv3, but is a very common extension.
// unfortunately, MacOS X only defines MAP_ANON, which Solaris says is
// deprecated. workaround there: define MAP_ANONYMOUS in terms of MAP_ANON.
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static const int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS;

LibError mem_Reserve(size_t size, u8** pp)
{
	errno = 0;
	void* ret = mmap(0, size, PROT_NONE, mmap_flags|MAP_NORESERVE, -1, 0);
	*pp = (u8*)ret;
	return LibError_from_mmap(ret);
}

LibError mem_Release(u8* p, size_t size)
{
	errno = 0;
	int ret = munmap(p, size);
	return LibError_from_posix(ret);
}

LibError mem_Commit(u8* p, size_t size, int prot)
{
	// avoid misinterpretation by mmap.
	if(prot == PROT_NONE)
		WARN_RETURN(ERR::INVALID_PARAM);

	errno = 0;
	void* ret = mmap(p, size, prot, mmap_flags|MAP_FIXED, -1, 0);
	return LibError_from_mmap(ret);
}

LibError mem_Decommit(u8* p, size_t size)
{
	errno = 0;
	void* ret = mmap(p, size, PROT_NONE, mmap_flags|MAP_NORESERVE|MAP_FIXED, -1, 0);
	return LibError_from_mmap(ret);
}

LibError mem_Protect(u8* p, size_t size, int prot)
{
	errno = 0;
	int ret = mprotect(p, size, prot);
	return LibError_from_posix(ret);
}


//-----------------------------------------------------------------------------

// "freelist" is a pointer to the first unused element (0 if there are none);
// its memory holds a pointer to the next free one in list.

void mem_freelist_AddToFront(void*& freelist, void* el)
{
	debug_assert(el != 0);

	void* prev_el = freelist;
	freelist = el;
	*(void**)el = prev_el;
}


void* mem_freelist_Detach(void*& freelist)
{
	void* el = freelist;
	// nothing in list
	if(!el)
		return 0;
	freelist = *(void**)el;
	return el;
}
