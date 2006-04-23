/**
 * =========================================================================
 * File        : win.cpp
 * Project     : 0 A.D.
 * Description : various Windows-specific code and program entry point.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include <stdio.h>
#include <stdlib.h>	// __argc

#include "win_internal.h"
#include "lib/path_util.h"

#if MSC_VERSION >= 1400
#include <process.h>	// __security_init_cookie
#define NEED_COOKIE_INIT
#endif


char win_sys_dir[MAX_PATH+1];
char win_exe_dir[MAX_PATH+1];


// only call after a Win32 function indicates failure.
static LibError LibError_from_GLE(bool warn_if_failed = true)
{
	LibError err;
	switch(GetLastError())
	{
	case ERROR_OUTOFMEMORY:
		err =  ERR_NO_MEM; break;

	case ERROR_INVALID_PARAMETER:
		err =  ERR_INVALID_PARAM; break;
	case ERROR_INSUFFICIENT_BUFFER:
		err =  ERR_BUF_SIZE; break;

	case ERROR_ACCESS_DENIED:
		err =  ERR_FILE_ACCESS; break;
	case ERROR_FILE_NOT_FOUND:
		err =  ERR_FILE_NOT_FOUND; break;
	case ERROR_PATH_NOT_FOUND:
		err =  ERR_PATH_NOT_FOUND; break;

	default:
		err =  ERR_FAIL; break;
	}

	if(warn_if_failed)
		DEBUG_WARN_ERR(err);
	return err;
}


// return the LibError equivalent of GetLastError(), or ERR_FAIL if
// there's no equal.
// you should SetLastError(0) before calling whatever will set ret
// to make sure we do not return any stale errors.
LibError LibError_from_win32(DWORD ret, bool warn_if_failed)
{
	if(ret != FALSE)
		return ERR_OK;
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


///////////////////////////////////////////////////////////////////////////////
//
// module init and shutdown mechanism
//
///////////////////////////////////////////////////////////////////////////////


// init and shutdown mechanism: register a function to be called at
// pre-libc init, pre-main init or shutdown.
//
// each module has the linker add a pointer to its init or shutdown
// function to a table (at a user-defined position).
// zero runtime overhead, and there's no need for a central dispatcher
// that knows about all the modules.
//
// disadvantage: requires compiler support (MS VC-specific).
//
// alternatives:
// - initialize via constructor. however, that would leave the problem of
//   shutdown order and timepoint, which is also taken care of here.
// - register init/shutdown functions from a NLSO constructor:
//   clunky, and setting order is more difficult.
// - on-demand initialization: complicated; don't know in what order
//   things happen. also, no way to determine how long init takes.
//
// the "segment name" determines when and in what order the functions are
// called: "LIB$W{type}{group}", where {type} is C for pre-libc init,
// I for pre-main init, or T for terminators (last of the atexit handlers).
// {group} is [B, Y]; groups are called in alphabetical order, but
// call order within the group itself is unspecified.
//
// define the segment via #pragma data_seg(name), register any functions
// to be called via WIN_REGISTER_FUNC, and then restore the previous segment
// with #pragma data_seg() .
//
// note: group must be [B, Y]. data declared in groups A or Z may
// be placed beyond the table start/end by the linker, since the linker's
// ordering WRT other source files' data is undefined within a segment.

typedef LibError (*_PIFV)(void);

// pointers to start and end of function tables.
// note: COFF tosses out empty segments, so we have to put in one value
// (zero, because call_func_tbl has to ignore NULL entries anyway).
#pragma data_seg(WIN_CALLBACK_PRE_LIBC(a))
_PIFV pre_libc_begin[] = { 0 };
#pragma data_seg(WIN_CALLBACK_PRE_LIBC(z))
_PIFV pre_libc_end[] = { 0 };
#pragma data_seg(WIN_CALLBACK_PRE_MAIN(a))
_PIFV pre_main_begin[] = { 0 };
#pragma data_seg(WIN_CALLBACK_PRE_MAIN(z))
_PIFV pre_main_end[] = { 0 };
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(a))
_PIFV shutdown_begin[] = { 0 };
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(z))
_PIFV shutdown_end[] = { 0 };
#pragma data_seg()

#pragma comment(linker, "/merge:.LIB=.data")

// call all non-NULL function pointers in [begin, end).
// note: the range may be larger than expected due to section padding.
// that (and the COFF empty section problem) is why we need to ignore zeroes.
static void call_func_tbl(_PIFV* begin, _PIFV* end)
{
	for(_PIFV* p = begin; p < end; p++)
		if(*p)
			(*p)();
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


static void cs_init()
{
	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);

	cs_valid = true;
}

static void cs_shutdown()
{
	cs_valid = false;

	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
	memset(cs, 0, sizeof(cs));
}


//-----------------------------------------------------------------------------
// startup
//-----------------------------------------------------------------------------

// entry -> pre_libc -> WinMainCRTStartup -> WinMain -> pre_main -> main
// at_exit is called as the last of the atexit handlers
// (assuming, as documented in lib.cpp, constructors don't use atexit!)
//
// rationale: we need to gain control after _cinit and before main() to
// complete initialization.
// note: this way of getting control before main adds overhead
// (setting up the WinMain parameters), but is simpler and safer than
// SDL-style #define main SDL_main.

// explained where used.
static HMODULE hUser32Dll;

static void at_exit(void)
{
	call_func_tbl(shutdown_begin, shutdown_end);

	cs_shutdown();

	// free the reference taken in win_pre_libc_init;
	// this avoids Boundschecker warnings at exit.
	FreeLibrary(hUser32Dll);
}


#ifndef NO_MAIN_REDIRECT
static
#endif
void win_pre_main_init()
{
	// enable memory tracking and leak detection;
	// no effect if !HAVE_VC_DEBUG_ALLOC.
#if CONFIG_PARANOIA
	debug_heap_enable(DEBUG_HEAP_ALL);
#elif !defined(NDEBUG)
	debug_heap_enable(DEBUG_HEAP_NORMAL);
#endif

	call_func_tbl(pre_main_begin, pre_main_end);

	atexit(at_exit);

	// no point redirecting stdout yet - the current directory
	// may be incorrect (file_set_root not yet called).
	// (w)sdl will take care of it anyway.
}


#ifndef NO_MAIN_REDIRECT

#undef main
extern int app_main(int argc, char* argv[]);
int main(int argc, char* argv[])
{
	win_pre_main_init();
	return app_main(argc, argv);
}
#endif


// perform all initialization that needs to run before _cinit
// (which calls C++ ctors).
// be very careful to avoid non-stateless libc functions!
static inline void pre_libc_init()
{
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

	cs_init();

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

	call_func_tbl(pre_libc_begin, pre_libc_end);
}


static int SEH_wrapped_entry()
{
	int ret;
	__try
	{
		pre_libc_init();
#ifdef USE_WINMAIN
		ret = WinMainCRTStartup();	// calls _cinit and then our main
#else
		ret = mainCRTStartup();	// calls _cinit and then our main
#endif
	}
	__except(wdbg_exception_filter(GetExceptionInformation()))
	{
		ret = -1;
	}
	return ret;
}


int entry()
{
#ifdef NEED_COOKIE_INIT
	// 2006-02-16 workaround for R6035 on VC8:
	//
	// SEH code compiled with /GS pushes a "security cookie" onto the
	// stack. since we're called before _cinit, the cookie won't have
	// been initialized yet, which would cause the CRT to FatalAppExit.
	// to solve this, we must call __security_init_cookie before any
	// hidden compiler-generated SEH registration code runs,
	// which means the __try block must be moved into a helper function.
	//
	// NB: entry() must not contain local string buffers, either -
	// /GS would install a cookie here as well (same problem).
	//
	// see http://msdn2.microsoft.com/en-US/library/ms235603.aspx
	__security_init_cookie();
#endif
	return SEH_wrapped_entry();
}
