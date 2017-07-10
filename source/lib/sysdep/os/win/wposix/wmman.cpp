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
#include "lib/sysdep/os/win/wposix/wmman.h"

#include "lib/sysdep/os/win/wposix/wposix_internal.h"
#include "lib/sysdep/os/win/wposix/crt_posix.h"		// _get_osfhandle


unsigned MemoryProtectionFromPosix(int prot)
{
	if(prot == PROT_NONE)
		return PAGE_NOACCESS;

	// this covers all 8 combinations of read|write|exec
	switch(prot & (PROT_READ|PROT_WRITE|PROT_EXEC))
	{
	case PROT_READ:
		return PAGE_READONLY;
	case PROT_WRITE:
		// not supported by Win32; POSIX allows us to also grant read access.
		return PAGE_READWRITE;
	case PROT_EXEC:
		return PAGE_EXECUTE;
	case PROT_READ|PROT_WRITE:
		return PAGE_READWRITE;
	case PROT_READ|PROT_EXEC:
		return PAGE_EXECUTE_READ;
	case PROT_WRITE|PROT_EXEC:
		// not supported by Win32; POSIX allows us to also grant read access.
		return PAGE_EXECUTE_READWRITE;
	case PROT_READ|PROT_WRITE|PROT_EXEC:
		return PAGE_EXECUTE_READWRITE;
	default:	// none set
		DEBUG_WARN_ERR(ERR::INVALID_FLAG);
		return PAGE_NOACCESS;
	}

	// UNREACHABLE
}


//-----------------------------------------------------------------------------
// memory mapping
//-----------------------------------------------------------------------------

int mprotect(void* addr, size_t len, int prot)
{
	const DWORD newProtect = (DWORD)MemoryProtectionFromPosix(prot);
	DWORD oldProtect;	// required by VirtualProtect
	const BOOL ok = VirtualProtect(addr, len, newProtect, &oldProtect);
	WARN_IF_FALSE(ok);
	return ok? 0 : -1;
}


// called when flags & MAP_ANONYMOUS
static Status mmap_mem(void* start, size_t len, int prot, int flags, int fd, void** pp)
{
	// sanity checks. we don't care about these but enforce them to
	// ensure callers are compatible with mmap.
	// .. MAP_ANONYMOUS is documented to require this.
	ENSURE(fd == -1);
	// .. if MAP_SHARED, writes are to change "the underlying [mapped]
	//    object", but there is none here (we're backed by the page file).
	ENSURE(!(flags & MAP_SHARED));

	// see explanation at MAP_NORESERVE definition.
	bool want_commit = (prot != PROT_NONE && !(flags & MAP_NORESERVE));

	// decommit a given area (leaves its address space reserved)
	if(!want_commit && start != 0 && flags & MAP_FIXED)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if(!VirtualQuery(start, &mbi, sizeof(mbi)))
			WARN_RETURN(StatusFromWin());
		if(mbi.State == MEM_COMMIT)
		{
			WARN_IF_FALSE(VirtualFree(start, len, MEM_DECOMMIT));
			*pp = 0;
			// make sure *pp won't be misinterpreted as an error
			cassert(MAP_FAILED);
			return INFO::OK;
		}
	}

	const DWORD allocationType = want_commit? MEM_COMMIT : MEM_RESERVE;
	const DWORD protect = (DWORD)MemoryProtectionFromPosix(prot);
	void* p = VirtualAlloc(start, len, allocationType, protect);
	if(!p)
	{
		debug_printf("wmman: VirtualAlloc(%p, 0x%I64X) failed\n", start, len);
		WARN_RETURN(ERR::NO_MEM);
	}
	*pp = p;
	return INFO::OK;
}


// given mmap prot and flags, output protection/access values for use with
// CreateFileMapping / MapViewOfFile. they only support read-only,
// read/write and copy-on-write, so we dumb it down to that and later
// set the correct (and more restrictive) permission via mprotect.
static Status DecodeFlags(int prot, int flags, DWORD& protect, DWORD& access)
{
	// ensure exactly one of (MAP_SHARED, MAP_PRIVATE) is specified
	switch(flags & (MAP_SHARED|MAP_PRIVATE))
	{
	case 0:
	case MAP_SHARED|MAP_PRIVATE:
		WARN_RETURN(ERR::INVALID_PARAM);
	default:;
	}

	if(prot & PROT_WRITE)
	{
		// determine write behavior: (whether they change the underlying file)
		if(flags & MAP_SHARED)	// writes affect the file
		{
			protect = PAGE_READWRITE;
			access  = FILE_MAP_WRITE;	// read and write
		}
		else	// copy on write (file remains unchanged)
		{
			protect = PAGE_WRITECOPY;
			access  = FILE_MAP_COPY;
		}
	}
	else
	{
		protect = PAGE_READONLY;
		access  = FILE_MAP_READ;
	}

	return INFO::OK;
}


static Status mmap_file(void* start, size_t len, int prot, int flags, int fd, off_t ofs, void** pp)
{
	WinScopedPreserveLastError s;

	ENSURE(fd != -1);	// handled by mmap_mem

	HANDLE hFile = HANDLE_from_intptr(_get_osfhandle(fd));
	if(hFile == INVALID_HANDLE_VALUE)
		WARN_RETURN(ERR::INVALID_HANDLE);

	// MapViewOfFileEx will fail if the "suggested" base address is
	// nonzero but cannot be honored, so wipe out <start> unless MAP_FIXED.
	if(!(flags & MAP_FIXED))
		start = 0;

	// choose protection and access rights for CreateFileMapping /
	// MapViewOfFile. these are weaker than what PROT_* allows and
	// are augmented below by subsequently mprotect-ing.
	DWORD protect; DWORD access;
	RETURN_STATUS_IF_ERR(DecodeFlags(prot, flags, protect, access));

	const HANDLE hMap = CreateFileMapping(hFile, 0, protect, 0, 0, 0);
	if(!hMap)
		WARN_RETURN(ERR::NO_MEM);
	void* p = MapViewOfFileEx(hMap, access, u64_hi(ofs), u64_lo(ofs), (SIZE_T)len, start);
	// ensure we got the requested address if MAP_FIXED was passed.
	ENSURE(!(flags & MAP_FIXED) || (p == start));
	// free the mapping object now, so that we don't have to hold on to its
	// handle until munmap(). it's not actually released yet due to the
	// reference held by MapViewOfFileEx (if it succeeded).
	CloseHandle(hMap);
	// map failed; bail now to avoid "restoring" the last error value.
	if(!p)
		WARN_RETURN(ERR::NO_MEM);

	// enforce the desired (more restrictive) protection.
	(void)mprotect(p, len, prot);

	*pp = p;
	return INFO::OK;
}


void* mmap(void* start, size_t len, int prot, int flags, int fd, off_t ofs)
{
	ASSERT(len != 0);

	void* p;
	Status status;
	if(flags & MAP_ANONYMOUS)
		status = mmap_mem(start, len, prot, flags, fd, &p);
	else
		status = mmap_file(start, len, prot, flags, fd, ofs, &p);
	if(status < 0)
	{
		errno = ErrnoFromStatus(status);
		return MAP_FAILED;	// NOWARN - already done
	}

	return p;
}


int munmap(void* start, size_t UNUSED(len))
{
	// UnmapViewOfFile checks if start was returned by MapViewOfFile*;
	// if not, it will fail.
	BOOL ok = UnmapViewOfFile(start);
	if(!ok)
		// VirtualFree requires dwSize to be 0 (entire region is released).
		ok = VirtualFree(start, 0, MEM_RELEASE);
	WARN_IF_FALSE(ok);
	return ok? 0 : -1;
}
