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
 * emulate a subset of POSIX on Win32.
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wposix/wposix.h"

#include "lib/sysdep/os/win/wposix/wposix_internal.h"
#include "lib/bits.h"

WINIT_REGISTER_CRITICAL_INIT(wposix_Init);	// wposix -> error handling


int setenv(const char* envname, const char* envval, int overwrite)
{
	if(!envname || envname[0] == '\0' || strchr(envname, '='))
	{
		errno = EINVAL;
		return -1;
	}

	if(overwrite || !getenv(envname))
		SetEnvironmentVariable(envname, envval);

	return 0;
}


//-----------------------------------------------------------------------------
// sysconf

// used by _SC_PAGESIZE and _SC_*_PAGES
static DWORD pageSize;
static DWORD numProcessors;
typedef BOOL (WINAPI *PGlobalMemoryStatusEx)(MEMORYSTATUSEX*);  
static PGlobalMemoryStatusEx pGlobalMemoryStatusEx;

// NB: called from critical init
static void InitSysconf()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);	// can't fail
	pageSize      = si.dwPageSize;	// used by _SC_PAGESIZE and _SC_*_PAGES
	numProcessors = si.dwNumberOfProcessors;

	// import GlobalMemoryStatusEx - it's not defined by the VC6 PSDK.
	// used by _SC_*_PAGES if available (provides better results).
	const HMODULE hKernel32Dll = GetModuleHandle("kernel32.dll");  
	pGlobalMemoryStatusEx = (PGlobalMemoryStatusEx)GetProcAddress(hKernel32Dll, "GlobalMemoryStatusEx"); 
}

long sysconf(int name)
{
	// called before InitSysconf => winit/wstartup are broken. this is
	// going to cause a hard crash because debug.cpp's error reporting
	// code requires the page size to be known. we'll raise an exception
	// with a unique value so that this issue is immediately noticed.
	if(!pageSize)
		RaiseException(0xFA57FA57, 0, 0, 0);

	switch(name)
	{
	case _SC_PAGESIZE:
	// note: no separate case for _SC_PAGE_SIZE - they are
	// different names but have the same value.
		return pageSize;

	case _SC_PHYS_PAGES:
	case _SC_AVPHYS_PAGES:
		{
		u64 total_phys_mem;
		u64 avail_phys_mem;

		// first query GlobalMemoryStatus - cannot fail.
		// override its results if GlobalMemoryStatusEx is available.
		{
		MEMORYSTATUS ms;
		GlobalMemoryStatus(&ms);
		total_phys_mem = ms.dwTotalPhys;
		avail_phys_mem = ms.dwAvailPhys;
		}

		// newer API is available: use it to report correct results
		// (no overflow or wraparound) on systems with > 4 GB of memory.
		{
		MEMORYSTATUSEX mse = { sizeof(mse) };
		if(pGlobalMemoryStatusEx && pGlobalMemoryStatusEx(&mse))
		{
			total_phys_mem = mse.ullTotalPhys;
			avail_phys_mem = mse.ullAvailPhys;
		}
		// else: not an error, since this isn't available before Win2k / XP.
		// we have results from GlobalMemoryStatus anyway.
		}

		// Richter, "Programming Applications for Windows": the reported
		// value doesn't include non-paged pool reserved during boot;
		// it's not considered available to kernel. (size is 528 KiB on
		// a 512 MiB WinXP/Win2k machine)
		// something similar may happen on other OSes, so it is fixed
		// by cpu.cpp instead of here.

		if(name == _SC_PHYS_PAGES)
			return (long)(total_phys_mem / pageSize);
 		else
			return (long)(avail_phys_mem / pageSize);
		}

	case _SC_NPROCESSORS_CONF:
		return numProcessors;

	default:
		return -1;
	}
}


//-----------------------------------------------------------------------------

static LibError wposix_Init()
{
	InitSysconf();
	return INFO::OK;
}
