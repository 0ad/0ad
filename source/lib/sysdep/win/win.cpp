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

#include "lib.h"
#include "win_internal.h"

#include <stdlib.h>	// __argc
#include <cstdio>
#include <cassert>

#include <crtdbg.h>	// malloc debug


//
// these override the portable stdio versions in sysdep.cpp
// (they're more convenient)
//

void display_msg(const wchar_t* caption, const wchar_t* msg)
{
	MessageBoxW(0, msg, caption, MB_ICONEXCLAMATION);
}


// fixing Win32 _vsnprintf to return # characters that would be written,
// as required by C99, looks difficult and unnecessary. if any other code
// needs that, generalize the following code into vasprintf.

void debug_out(const char* fmt, ...)
{
	size_t size = 256;
	void* buf = 0;

	for(;;)
	{
		void* buf2 = realloc(buf, size);
		// out of mem - free old buf and quit
		if(!buf2)
			goto fail;
		buf = buf2;
			// don't assign directly from realloc - if it fails,
			// we'd leak memory.

		{
		va_list args;
		va_start(args, fmt);
			// have to re-create every time. va_copy isn't portable.

		int ret = vsnprintf((char*)buf, size, fmt, args);

		// success
		if(ret > -1 && ret < (int)size)
			break;
		// return value was required buffer size (without trailing '\0')
		if(ret > (int)size)
			size = ret+1;
		// -1: increase buffer size
		else
			size *= 2;

		va_end(args);
		}

		// prevent infinite loop
		if(size > 64*KB)
			goto fail;
	}

	OutputDebugString((const char*)buf);

fail:
	free(buf);
}
	



#ifndef NO_WINSOCK
#pragma comment(lib, "ws2_32.lib")
#endif

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
	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
}


// be very careful to avoid non-stateless libc functions!
static inline void pre_libc_init()
{
#ifndef NO_WINSOCK
	char d[1024];
	WSAStartup(0x0002, d);	// want 2.0
	atexit2(WSACleanup, 0, CC_STDCALL_0);
#endif

	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);
}


static inline void pre_main_init()
{
#ifdef PARANOIA
	// force malloc et al to check the heap every call.
	// slower, but reports errors closer to where they occur.
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	flags |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;
	_CrtSetDbgFlag(flags);
#endif

	atexit(at_exit);

	// SDL will do this as well. no matter.
	// ignore BoundsChecker warning here.
	freopen("stdout.txt", "wt", stdout);
}




int entry()
{
	pre_libc_init();
	return WinMainCRTStartup();	// calls _cinit, and then WinMain
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	pre_main_init();
	return main(__argc, __argv);
}