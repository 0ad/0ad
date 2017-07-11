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

#include "precompiled.h"
#include "lib/sysdep/vm.h"

#include "lib/alignment.h"

// "anonymous" effectively means mapping /dev/zero, but is more efficient.
// MAP_ANONYMOUS is not in SUSv3, but is a very common extension.
// unfortunately, MacOS X only defines MAP_ANON, which Solaris says is
// deprecated. workaround there: define MAP_ANONYMOUS in terms of MAP_ANON.
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static const int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS;

namespace vm {

void* ReserveAddressSpace(size_t size, size_t UNUSED(commitSize), PageType UNUSED(pageType), int UNUSED(prot))
{
	errno = 0;
	void* p = mmap(0, size, PROT_NONE, mmap_flags|MAP_NORESERVE, -1, 0);
	if(p == MAP_FAILED)
		return 0;
	return p;
}

void ReleaseAddressSpace(void* p, size_t size)
{
	ENSURE(size != 0);

	errno = 0;
	if(munmap(p, size) != 0)
		DEBUG_WARN_ERR(StatusFromErrno());
}


bool Commit(uintptr_t address, size_t size, PageType UNUSED(pageType), int prot)
{
	if(prot == PROT_NONE)	// would be understood as a request to decommit
	{
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return false;
	}

	errno = 0;
	if(mmap((void*)address, size, prot, mmap_flags|MAP_FIXED, -1, 0) == MAP_FAILED)
		return false;

	if(prot != (PROT_READ|PROT_WRITE))
		(void)Protect(address, size, prot);

	return true;
}

bool Decommit(uintptr_t address, size_t size)
{
	errno = 0;
	if(mmap((void*)address, size, PROT_NONE, mmap_flags|MAP_NORESERVE|MAP_FIXED, -1, 0) == MAP_FAILED)
		return false;
	return true;
}


bool Protect(uintptr_t address, size_t size, int prot)
{
	errno = 0;
	if(mprotect((void*)address, size, prot) != 0)
	{
		DEBUG_WARN_ERR(ERR::FAIL);
		return false;
	}
	return true;
}


void* Allocate(size_t size, PageType pageType, int prot)
{
	void* p = ReserveAddressSpace(size);
	if(!p)
		return 0;

	if(!Commit(uintptr_t(p), size, pageType, prot))
	{
		ReleaseAddressSpace(p, size);
		return 0;
	}

	return p;
}

void Free(void* p, size_t size)
{
	// (only the Windows implementation distinguishes between Free and ReleaseAddressSpace)
	vm::ReleaseAddressSpace(p, size);
}


void BeginOnDemandCommits()
{
	// not yet implemented, but possible with a signal handler
}

void EndOnDemandCommits()
{
	// not yet implemented, but possible with a signal handler
}


void DumpStatistics()
{
	// we haven't collected any statistics
}

}	// namespace vm
