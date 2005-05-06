// Windows-specific code and program entry point
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "lib.h"
#include "posix.h"
#include "win_internal.h"

#include <crtdbg.h>	// malloc debug

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>	// __argc
#include <malloc.h>
#include <shlobj.h>	// pick_dir

#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")	// for pick_directory SH* calls
#endif


void sle(int x)
{
	SetLastError((DWORD)x);
}


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


char win_sys_dir[MAX_PATH+1];
char win_exe_dir[MAX_PATH+1];


//
// these override the portable versions in sysdep.cpp
// (they're more convenient)
//


inline void debug_check_heap()
{
	_heapchk();
}


inline int get_executable_name(char* n_path, size_t buf_size)
{
	DWORD nbytes = GetModuleFileName(0, n_path, (DWORD)buf_size);
	return nbytes? 0 : -1;
}


static int CALLBACK browse_cb(HWND hWnd, unsigned int msg, LPARAM lParam, LPARAM ldata)
{
	UNUSED(lParam);
	if(msg == BFFM_INITIALIZED)
	{
		const char* cur_dir = (const char*)ldata;
		SendMessage(hWnd, BFFM_SETSELECTIONA, 1, (LPARAM)cur_dir);
		return 1;
	}

	return 0;
}

int pick_directory(char* path, size_t buf_size)
{
	assert(buf_size >= PATH_MAX);
	IMalloc* p_malloc;
	SHGetMalloc(&p_malloc);

	GetCurrentDirectory(PATH_MAX, path);

///	ShowWindow(hWnd, SW_HIDE);

	BROWSEINFOA bi;
	memset(&bi, 0, sizeof(bi));
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = (BFFCALLBACK)browse_cb;
	bi.lParam = (LPARAM)path;
	ITEMIDLIST* pidl = SHBrowseForFolderA(&bi);
	BOOL ok = SHGetPathFromIDList(pidl, path);

///	ShowWindow(hWnd, SW_SHOW);

	p_malloc->Free(pidl);
	p_malloc->Release();

	return ok? 0 : -1;
}


void display_msg(const char* caption, const char* msg)
{
	MessageBoxA(0, msg, caption, MB_ICONEXCLAMATION);
}

void wdisplay_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION);
}










int clipboard_set(const wchar_t* text)
{
	int err = -1;

	const HWND new_owner = 0;
		// MSDN: passing 0 requests the current task be granted ownership;
		// there's no need to pass our window handle.
	if(!OpenClipboard(new_owner))
		return err;
	EmptyClipboard();

	err = 0;

	const size_t len = wcslen(text);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len+1) * sizeof(wchar_t));
	if(!hMem)
		goto fail;

	wchar_t* copy = (wchar_t*)GlobalLock(hMem);
	if(copy)
	{
		wcscpy(copy, text);

		GlobalUnlock(hMem);

		if(SetClipboardData(CF_UNICODETEXT, hMem) != 0)
			err = 0;	// success
	}

fail:
	CloseClipboard();
	return err;
}


wchar_t* clipboard_get()
{
	wchar_t* ret = 0;

	const HWND new_owner = 0;
		// MSDN: passing 0 requests the current task be granted ownership;
		// there's no need to pass our window handle.
	if(!OpenClipboard(new_owner))
		return 0;

	// Windows NT/2000+ auto convert UNICODETEXT <-> TEXT
	HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
	if(hMem != 0)
	{
		wchar_t* text = (wchar_t*)GlobalLock(hMem);
		if(text)
		{
			SIZE_T size = GlobalSize(hMem);
			wchar_t* copy = (wchar_t*)malloc(size);
			if(copy)
			{
				wcscpy(copy, text);
				ret = copy;
			}

			GlobalUnlock(hMem);
		}
	}

	CloseClipboard();

	return ret;
}


int clipboard_free(wchar_t* copy)
{
	free(copy);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// init and shutdown mechanism
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

typedef int(*_PIFV)(void);

// pointers to start and end of function tables.
// note: COFF throws out empty segments, so we have to put in one value
// (zero, because call_func_tbl has to ignore NULL entries anyway).
#pragma data_seg(".LIB$WCA")
_PIFV pre_libc_begin[] = { 0 };
#pragma data_seg(".LIB$WCZ")
_PIFV pre_libc_end[] = { 0 };
#pragma data_seg(".LIB$WIA")
_PIFV pre_main_begin[] = { 0 };
#pragma data_seg(".LIB$WIZ")
_PIFV pre_main_end[] = { 0 };
#pragma data_seg(".LIB$WTA")
_PIFV shutdown_begin[] = { 0 };
#pragma data_seg(".LIB$WTZ")
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


///////////////////////////////////////////////////////////////////////////////


// locking for win-specific code
// several init functions are before _cinit.
// can't guarantee POSIX static mutex init has been done by then.



static CRITICAL_SECTION cs[NUM_CS];
static bool cs_valid;

void win_lock(uint idx)
{
	assert(idx < NUM_CS && "win_lock: invalid critical section index");
	if(cs_valid)
		EnterCriticalSection(&cs[idx]);
}

void win_unlock(uint idx)
{
	assert(idx < NUM_CS && "win_unlock: invalid critical section index");
	if(cs_valid)
		LeaveCriticalSection(&cs[idx]);
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


///////////////////////////////////////////////////////////////////////////////


// entry -> pre_libc -> WinMainCRTStartup -> WinMain -> pre_main -> main
// at_exit is called as the last of the atexit handlers
// (assuming, as documented in lib.cpp, constructors don't use atexit!)
//
// note: this way of getting control before main adds overhead
// (setting up the WinMain parameters), but is simpler and safer than
// SDL-style #define main SDL_main.

static void at_exit(void)
{
	call_func_tbl(shutdown_begin, shutdown_end);

	cs_shutdown();
}


// be very careful to avoid non-stateless libc functions!
static inline void pre_libc_init()
{
#if WINVER >= 0x0501
	// enable low-fragmentation heap
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
	{
		char* slash = strrchr(win_exe_dir, '\\');
		if(slash)
			*slash = '\0';
	}

	call_func_tbl(pre_libc_begin, pre_libc_end);
}

static inline void pre_main_init()
{
#ifdef HAVE_DEBUGALLOC
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	// Always enable leak detection in debug builds
	flags |= _CRTDBG_LEAK_CHECK_DF;
#ifdef PARANOIA
	// force malloc et al. to check the heap every call.
	// slower, but reports errors closer to where they occur.
	flags |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;
#endif // PARANOIA
	_CrtSetDbgFlag(flags);
#endif // HAVE_DEBUGALLOC

	call_func_tbl(pre_main_begin, pre_main_end);

	atexit(at_exit);

	// no point redirecting stdout yet - the current directory
	// may be incorrect (file_set_root not yet called).
	// (w)sdl will take care of it anyway.
}

#ifdef HAVE_DEBUGALLOC
// Enable heap corruption checking after every allocation. Has the same
// effect as PARANOIA in pre_main_init, but lets you switch it on anywhere
// so that you can skip checking the whole of the initialisation code.
// The debugger will break in the allocation just after the one that
// corrupted the heap, so check its ID and then _CrtSetBreakAlloc(...)
// on the previous one and try again.
// Warning: This makes things rather slow.
void memory_debug_extreme_turbo_plus()
{
	_CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF );
}
#endif

int entry()
{
	pre_libc_init();
	return WinMainCRTStartup();	// calls _cinit, and then WinMain
}


#ifdef SCED
void sced_init()
{
	pre_main_init();
}
#endif

#ifndef SCED
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	pre_main_init();
	return main(__argc, __argv);
}
#endif