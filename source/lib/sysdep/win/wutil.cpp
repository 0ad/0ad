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
#include "win.h"
#include "winit.h"


#pragma SECTION_INIT(1)	// early, several modules depend on us
WINIT_REGISTER_FUNC(wutil_PreLibcInit);
#pragma FORCE_INCLUDE(wutil_PreLibcInit)
#pragma SECTION_SHUTDOWN(8)
WINIT_REGISTER_FUNC(wutil_Shutdown);
#pragma FORCE_INCLUDE(wutil_Shutdown)
#pragma SECTION_RESTORE


//-----------------------------------------------------------------------------
// safe allocator

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
// locks

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
// error codes

// only call after a Win32 function indicates failure.
LibError LibError_from_GLE(bool warn_if_failed)
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
// directories

char win_sys_dir[MAX_PATH+1];
char win_exe_dir[MAX_PATH+1];

static void GetDirectories()
{
	GetSystemDirectory(win_sys_dir, sizeof(win_sys_dir));

	if(GetModuleFileName(GetModuleHandle(0), win_exe_dir, MAX_PATH) != 0)
		path_strip_fn(win_exe_dir);
}


//-----------------------------------------------------------------------------
// user32 fix

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
static HMODULE hUser32Dll;

static void ForciblyLoadUser32Dll()
{
	hUser32Dll = LoadLibrary("user32.dll");
}

// avoids Boundschecker warning
static void FreeUser32Dll()
{
	FreeLibrary(hUser32Dll);
}


//-----------------------------------------------------------------------------
// memory

// note: has no effect if config.h's HAVE_VC_DEBUG_ALLOC is 0.
static void EnableMemoryTracking()
{
#if CONFIG_PARANOIA
	debug_heap_enable(DEBUG_HEAP_ALL);
#elif !defined(NDEBUG)
	debug_heap_enable(DEBUG_HEAP_NORMAL);
#endif
}

static void EnableLowFragmentationHeap()
{
#if WINVER >= 0x0501
	HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	BOOL (WINAPI* pHeapSetInformation)(HANDLE, HEAP_INFORMATION_CLASS, void*, size_t);
	*(void**)&pHeapSetInformation = GetProcAddress(hKernel32Dll, "HeapSetInformation");
	if(!pHeapSetInformation)
		return;

	ULONG flags = 2;	// enable LFH
	pHeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &flags, sizeof(flags));

	FreeLibrary(hKernel32Dll);
#endif	// #if WINVER >= 0x0501
}


//-----------------------------------------------------------------------------
// version

static char versionString[20];

static void DetectWindowsVersion()
{
	// note: don't use GetVersion[Ex] because it gives the version of the
	// emulated OS when running an app with compatibility shims enabled.
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		DWORD size = ARRAY_SIZE(versionString);
		(void)RegQueryValueEx(hKey, "CurrentVersion", 0, 0, (LPBYTE)versionString, &size);

		RegCloseKey(hKey);
	}
	else
		debug_assert(0);
}

const char* wutil_WindowsVersionString()
{
	debug_assert(versionString[0] != '\0');
	return versionString;
}

bool wutil_IsVista()
{
	debug_assert(versionString[0] != '\0');
	return (versionString[0] >= '6');
}

//-----------------------------------------------------------------------------
// Wow64

// Wow64 'helpfully' redirects all 32-bit apps' accesses of
// %windir%\\system32\\drivers to %windir%\\system32\\drivers\\SysWOW64.
// that's bad, because the actual drivers are not in the subdirectory. to
// work around this, provide for temporarily disabling redirection.

static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static BOOL (WINAPI *pWow64DisableWow64FsRedirection)(PVOID*) = 0;
static BOOL (WINAPI *pWow64RevertWow64FsRedirection)(PVOID) = 0;

static bool isWow64;

static void ImportWow64Functions()
{
	HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	*(void**)&pIsWow64Process = GetProcAddress(hKernel32Dll, "IsWow64Process"); 
	*(void**)&pWow64DisableWow64FsRedirection = GetProcAddress(hKernel32Dll, "Wow64DisableWow64FsRedirection");
	*(void**)&pWow64RevertWow64FsRedirection  = GetProcAddress(hKernel32Dll, "Wow64RevertWow64FsRedirection");
	FreeLibrary(hKernel32Dll);
}

static void DetectWow64()
{
	// function not found => running on 32-bit Windows
	if(!pIsWow64Process)
	{
		isWow64 = false;
		return;
	}

	BOOL isWow64Process = FALSE;
	const BOOL ok = IsWow64Process(GetCurrentProcess(), &isWow64Process);
	WARN_IF_FALSE(ok);
	isWow64 = (isWow64Process == TRUE);
}

bool wutil_IsWow64()
{
	return isWow64;
}

void wutil_DisableWow64Redirection(void*& wasRedirectionEnabled)
{
	// note: don't just check if the function pointers are valid. 32-bit
	// Vista includes them but isn't running Wow64, so calling the functions
	// would fail. since we have to check if actually on Wow64, there's no
	// more need to verify the pointers (their existence is implied).
	if(!wutil_IsWow64())
		return;
	BOOL ok = pWow64DisableWow64FsRedirection(&wasRedirectionEnabled);
	WARN_IF_FALSE(ok);
}

void wutil_RevertWow64Redirection(void* wasRedirectionEnabled)
{
	if(!wutil_IsWow64())
		return;
	BOOL ok = pWow64RevertWow64FsRedirection(wasRedirectionEnabled);
	WARN_IF_FALSE(ok);
}


//-----------------------------------------------------------------------------

static LibError wutil_PreLibcInit()
{
	InitLocks();

	ForciblyLoadUser32Dll();

	EnableMemoryTracking();

	EnableLowFragmentationHeap();

	GetDirectories();

	DetectWindowsVersion();

	ImportWow64Functions();
	DetectWow64();

	return INFO::OK;
}


static LibError wutil_Shutdown()
{
	FreeUser32Dll();

	ShutdownLocks();

	return INFO::OK;
}
