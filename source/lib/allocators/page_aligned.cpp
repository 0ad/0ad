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

#include "precompiled.h"
#include "lib/allocators/page_aligned.h"

#include "lib/alignment.h"
#include "lib/sysdep/cpu.h"	// cpu_CAS


//-----------------------------------------------------------------------------

static inline Status StatusFromMap(void* ret)
{
	if(ret != MAP_FAILED)
		return INFO::OK;
	WARN_RETURN(StatusFromErrno());
}

// "anonymous" effectively means mapping /dev/zero, but is more efficient.
// MAP_ANONYMOUS is not in SUSv3, but is a very common extension.
// unfortunately, MacOS X only defines MAP_ANON, which Solaris says is
// deprecated. workaround there: define MAP_ANONYMOUS in terms of MAP_ANON.
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static const int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS;

Status mem_Reserve(size_t size, u8** pp)
{
	errno = 0;
	void* ret = mmap(0, size, PROT_NONE, mmap_flags|MAP_NORESERVE, -1, 0);
	*pp = (u8*)ret;
	return StatusFromMap(ret);
}

Status mem_Release(u8* p, size_t size)
{
	errno = 0;
	if(munmap(p, size) != 0)
		WARN_RETURN(StatusFromErrno());
	return 0;
}

Status mem_Commit(u8* p, size_t size, int prot)
{
	// avoid misinterpretation by mmap.
	if(prot == PROT_NONE)
		WARN_RETURN(ERR::INVALID_PARAM);

	errno = 0;
	void* ret = mmap(p, size, prot, mmap_flags|MAP_FIXED, -1, 0);
	return StatusFromMap(ret);
}

Status mem_Decommit(u8* p, size_t size)
{
	errno = 0;
	void* ret = mmap(p, size, PROT_NONE, mmap_flags|MAP_NORESERVE|MAP_FIXED, -1, 0);
	return StatusFromMap(ret);
}

Status mem_Protect(u8* p, size_t size, int prot)
{
	errno = 0;
	if(mprotect(p, size, prot) != 0)
		WARN_RETURN(StatusFromErrno());
	return 0;

}


//-----------------------------------------------------------------------------

void* page_aligned_alloc(size_t size)
{
	const size_t alignedSize = Align<pageSize>(size);
	u8* p = 0;
	RETURN_0_IF_ERR(mem_Reserve(alignedSize, &p));
	RETURN_0_IF_ERR(mem_Commit(p, alignedSize, PROT_READ|PROT_WRITE));
	return p;
}


void page_aligned_free(void* p, size_t size)
{
	if(!p)
		return;
	ENSURE(IsAligned(p, pageSize));
	const size_t alignedSize = Align<pageSize>(size);
	(void)mem_Release((u8*)p, alignedSize);
}
