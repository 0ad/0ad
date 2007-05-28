/**
 * =========================================================================
 * File        : wposix.cpp
 * Project     : 0 A.D.
 * Description : emulate a subset of POSIX on Win32.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wposix.h"

#include "wposix_internal.h"
#include "crt_posix.h"		// _getcwd
#include "lib/bits.h"


#pragma SECTION_INIT(5)
WINIT_REGISTER_FUNC(wposix_Init);
#pragma FORCE_INCLUDE(wposix_Init)
#pragma SECTION_RESTORE


//-----------------------------------------------------------------------------
// sysconf

// used by _SC_PAGESIZE and _SC_*_PAGES
static DWORD page_size;
static BOOL (WINAPI *pGlobalMemoryStatusEx)(MEMORYSTATUSEX*);  

static void InitSysconf()
{
	// get page size
	// (used by _SC_PAGESIZE and _SC_*_PAGES)
	SYSTEM_INFO si;
	GetSystemInfo(&si);		// can't fail => page_size always > 0.
	page_size = si.dwPageSize;

	// import GlobalMemoryStatusEx - it's not defined by the VC6 PSDK.
	// used by _SC_*_PAGES if available (provides better results).
	const HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");  
	*(void**)&pGlobalMemoryStatusEx = GetProcAddress(hKernel32Dll, "GlobalMemoryStatusEx"); 
	FreeLibrary(hKernel32Dll);
}

long sysconf(int name)
{
	debug_assert(page_size);	// must not be called before InitSysconf

	switch(name)
	{
	case _SC_PAGESIZE:
	// note: no separate case for _SC_PAGE_SIZE - they are
	// different names but have the same value.
		return page_size;

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
			return (long)(total_phys_mem / page_size);
 		else
			return (long)(avail_phys_mem / page_size);
		}

	default:
		return -1;
	}
}


//-----------------------------------------------------------------------------

#ifdef REDEFINED_NEW
# include "lib/nommgr.h"
#endif
char* getcwd(char* buf, size_t buf_size)
{
	return _getcwd(buf, (int)buf_size);
}
#ifdef REDEFINED_NEW
# include "lib/mmgr.h"
#endif


//-----------------------------------------------------------------------------

static LibError wposix_Init()
{
	InitSysconf();
	return INFO::OK;
}
