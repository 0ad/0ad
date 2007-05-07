/**
 * =========================================================================
 * File        : wutil.cpp
 * Project     : 0 A.D.
 * Description : various Windows-specific utilities
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wutil.h"

#include <stdio.h>
#include <stdlib.h>	// __argc

#include "lib/path_util.h"
#include "lib/posix/posix.h"
#include "win_internal.h"
#include "winit.h"


#pragma SECTION_PRE_LIBC(B)
WIN_REGISTER_FUNC(wutil_PreLibcInit);
#pragma FORCE_INCLUDE(wutil_PreLibcInit)
#pragma SECTION_POST_ATEXIT(Y)
WIN_REGISTER_FUNC(wutil_Shutdown);
#pragma FORCE_INCLUDE(wutil_Shutdown)
#pragma SECTION_RESTORE


char win_sys_dir[MAX_PATH+1];
char win_exe_dir[MAX_PATH+1];


// only call after a Win32 function indicates failure.
static LibError LibError_from_GLE(bool warn_if_failed = true)
{
	LibError err;
	switch(GetLastError())
	{
	case ERROR_OUTOFMEMORY:
		err = ERR::NO_MEM; break;

	case ERROR_INVALID_PARAMETER:
		err = ERR::INVALID_PARAM; break;
	case ERROR_INSUFFICIENT_BUFFER:
		err = ERR::BUF_SIZE; break;

/*
	case ERROR_ACCESS_DENIED:
		err = ERR::FILE_ACCESS; break;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		err = ERR::TNODE_NOT_FOUND; break;
*/
	default:
		err = ERR::FAIL; break;
	}

	if(warn_if_failed)
		DEBUG_WARN_ERR(err);
	return err;
}


// return the LibError equivalent of GetLastError(), or ERR::FAIL if
// there's no equal.
// you should SetLastError(0) before calling whatever will set ret
// to make sure we do not return any stale errors.
LibError LibError_from_win32(DWORD ret, bool warn_if_failed)
{
	if(ret != FALSE)
		return INFO::OK;
	return LibError_from_GLE(warn_if_failed);
}


//-----------------------------------------------------------------------------

//
// safe allocator that may be used independently of libc malloc
// (in particular, before _cinit and while calling static dtors).
// used by wpthread critical section code.
//

void* win_alloc(size_t size)
{
	const DWORD flags = HEAP_ZERO_MEMORY;
	return HeapAlloc(GetProcessHeap(), flags, size);
}

void win_free(void* p)
{
	const DWORD flags = 0;
	HeapFree(GetProcessHeap(), flags, p);
}


//-----------------------------------------------------------------------------
// locking for win-specific code
//-----------------------------------------------------------------------------

// several init functions are before called before _cinit.
// POSIX static mutex init may not have been done by then,
// so we need our own lightweight functions.

static CRITICAL_SECTION cs[NUM_CS];
static bool cs_valid;

void win_lock(uint idx)
{
	debug_assert(idx < NUM_CS && "win_lock: invalid critical section index");
	if(cs_valid)
		EnterCriticalSection(&cs[idx]);
}

void win_unlock(uint idx)
{
	debug_assert(idx < NUM_CS && "win_unlock: invalid critical section index");
	if(cs_valid)
		LeaveCriticalSection(&cs[idx]);
}

int win_is_locked(uint idx)
{
	debug_assert(idx < NUM_CS && "win_is_locked: invalid critical section index");
	if(!cs_valid)
		return -1;
	BOOL got_it = TryEnterCriticalSection(&cs[idx]);
	if(got_it)
		LeaveCriticalSection(&cs[idx]);
	return !got_it;
}


static void InitLocks()
{
	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);

	cs_valid = true;
}

static void ShutdownLocks()
{
	cs_valid = false;

	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
	memset(cs, 0, sizeof(cs));
}


//-----------------------------------------------------------------------------

// explained where used.
static HMODULE hUser32Dll;


static LibError wutil_PreLibcInit()
{
	// enable memory tracking and leak detection;
	// no effect if !HAVE_VC_DEBUG_ALLOC.
#if CONFIG_PARANOIA
	debug_heap_enable(DEBUG_HEAP_ALL);
#elif !defined(NDEBUG)
	debug_heap_enable(DEBUG_HEAP_NORMAL);
#endif

	// enable low-fragmentation heap
#if WINVER >= 0x0501
	HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	if(hKernel32Dll)
	{
		BOOL (WINAPI* pHeapSetInformation)(HANDLE, HEAP_INFORMATION_CLASS, void*, size_t);
		*(void**)&pHeapSetInformation = GetProcAddress(hKernel32Dll, "HeapSetInformation");
		if(pHeapSetInformation)
		{
			ULONG flags = 2;	// enable LFH
			pHeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &flags, sizeof(flags));
		}

		FreeLibrary(hKernel32Dll);
	}
#endif	// #if WINVER >= 0x0501

	InitLocks();

	GetSystemDirectory(win_sys_dir, sizeof(win_sys_dir));

	if(GetModuleFileName(GetModuleHandle(0), win_exe_dir, MAX_PATH) != 0)
		path_strip_fn(win_exe_dir);

	// HACK: make sure a reference to user32 is held, even if someone
	// decides to delay-load it. this fixes bug #66, which was the
	// Win32 mouse cursor (set via user32!SetCursor) appearing as a
	// black 32x32(?) rectangle. underlying cause was as follows:
	// powrprof.dll was the first client of user32, causing it to be
	// loaded. after we were finished with powrprof, we freed it, in turn
	// causing user32 to unload. later code would then reload user32,
	// which apparently terminally confused the cursor implementation.
	//
	// since we hold a reference here, user32 will never unload.
	// of course, the benefits of delay-loading are lost for this DLL,
	// but that is unavoidable. it is safer to force loading it, rather
	// than documenting the problem and asking it not be delay-loaded.
	hUser32Dll = LoadLibrary("user32.dll");

	return INFO::OK;
}


static LibError wutil_Shutdown()
{
	ShutdownLocks();

	// free the reference taken in win_PreInit;
	// this avoids Boundschecker warnings at exit.
	FreeLibrary(hUser32Dll);

	return INFO::OK;
}
