// Windows-specific code
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
#include "win_internal.h"

#include <crtdbg.h>	// malloc debug

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>	// __argc
#include <malloc.h>

//
// these override the portable stdio versions in sysdep.cpp
// (they're more convenient)
//


void check_heap()
{
	_heapchk();
}


void display_msg(const char* caption, const char* msg)
{
	MessageBoxA(0, msg, caption, MB_ICONEXCLAMATION);
}

void wdisplay_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION);
}



// need to shoehorn printf-style variable params into
// the OutputDebugString call.
// - don't want to split into multiple calls - would add newlines to output.
// - fixing Win32 _vsnprintf to return # characters that would be written,
//   as required by C99, looks difficult and unnecessary. if any other code
//   needs that, implement GNU vasprintf.
// - fixed size buffers aren't nice, but much simpler than vasprintf-style
//   allocate+expand_until_it_fits. these calls are for quick debug output,
//   not loads of data, anyway.


static const int MAX_CNT = 512;
	// max output size of 1 call of (w)debug_out (including \0)



void debug_out(const char* fmt, ...)
{
	char buf[MAX_CNT];
	buf[MAX_CNT-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugString(buf);
}


void wdebug_out(const wchar_t* fmt, ...)
{
	wchar_t buf[MAX_CNT];
	buf[MAX_CNT-1] = L'\0';

	va_list ap;
	va_start(ap, fmt);
	vsnwprintf(buf, MAX_CNT-1, fmt, ap);
	va_end(ap);

	OutputDebugStringW(buf);
}






static Modules modules;


int add_module(int(*pre_main)(), int(*at_exit)())
{
	win_lock(WIN_CS);
	struct ModuleCallbacks cbs = { pre_main, at_exit };
	if(modules.count >= MAX_MODULES)
		debug_warn("increase MAX_MODULES");
	else
		modules.cbs[modules.count++] = cbs;
	win_unlock(WIN_CS);
	return 0;
}

static int call_module_funcs(bool pre_main)
{
	if(pre_main)
	{
		for(size_t i = 0; i < modules.count; i++)
			modules.cbs[i].pre_main();
	}
	else
	{
		size_t i = modules.count;
		for(;;)
		{
			modules.cbs[--i].at_exit();
			if(!i)
				break;
		}
	}

	return 0;
}





// locking for win-specific code
// several init functions are called on-demand, possibly from constructors.
// can't guarantee POSIX static mutex init has been done by then.



static CRITICAL_SECTION cs[NUM_CS];

void win_lock(uint idx)
{
	assert(idx < NUM_CS && "win_lock: invalid critical section index");
	EnterCriticalSection(&cs[idx]);
}

void win_unlock(uint idx)
{
	assert(idx < NUM_CS && "win_unlock: invalid critical section index");
	LeaveCriticalSection(&cs[idx]);
}



// entry -> pre_libc -> WinMainCRTStartup -> WinMain -> pre_main -> main
// at_exit is called as the last of the atexit handlers
// (assuming, as documented in lib.cpp, constructors don't use atexit!)
//
// note: this way of getting control before main adds overhead
// (setting up the WinMain parameters), but is simpler and safer than
// SDL-style #define main SDL_main.

static void at_exit(void)
{
	call_module_funcs(false);

	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
}


// be very careful to avoid non-stateless libc functions!
static inline void pre_libc_init()
{
	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);
}


static inline void pre_main_init()
{
#ifdef PARANOIA
	// force malloc et al to check the heap every call.
	// slower, but reports errors closer to where they occur.
	uint flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;
	_CrtSetDbgFlag(flags);
#endif

	call_module_funcs(true);

	atexit(at_exit);

	// no point redirecting stdout yet - the current directory
	// may be incorrect (file_set_root not yet called).
	// (w)sdl will take care of it anyway.
}


extern u64 rdtsc();
extern u64 PREVTSC;
u64 PREVTSC;

int entry()
{
#ifdef _MSC_VER
u64 TSC=rdtsc();
debug_out(
"----------------------------------------\n"\
"ENTRY\n"\
"----------------------------------------\n");
PREVTSC=TSC;
#endif

	pre_libc_init();
	return WinMainCRTStartup();	// calls _cinit, and then WinMain
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	pre_main_init();
	return main(__argc, __argv);
}







#pragma comment(lib, "delayimp.lib")
