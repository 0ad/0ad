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

#include "types.h"
#include "lib.h"

#include <cassert>
#include <cstdlib>


// more powerful atexit, with 0 or 1 parameters.
// callable before libc initialized, frees up the real atexit table,
// and often obviates a separate cleanup_everything function.
//
// problem: some of the functions registered here must be called after
// all other shutdown code (e.g. Winsock cleanup).
// we can't wedge ourselves between the regular atexit calls and
// process termination, so hooking exit isn't possible.
// need to use regular atexit, which must be called after _cinit.
// AFAIK, we can't interpose ourselves between libc init and constructors
// either, so constructors MUST NOT:
// - exit() (otherwise, some resources leak, because our atexit handler
//   wouldn't have been registered yet - it's done from main())
// - call atexit (our exit handler would be called before its handler,
//   so we may have shut down something important already).

const int MAX_EXIT_FUNCS = 8;


static struct ExitFunc
{
	void* func;
	uintptr_t arg;
	CallConvention cc;
}
exit_funcs[MAX_EXIT_FUNCS];
static int num_exit_funcs;


// call all registered exit handlers in LIFO order.
// called from exit, so don't worry about thread safety.
static void call_exit_funcs(void)
{
	ExitFunc* p = exit_funcs;
	for(int i = num_exit_funcs-1; i >= 0; i--)
	{
		switch(p->cc)
		{
		case CC_CDECL_0:
			((void(*)(void))p->func)();
			break;
		case CC_CDECL_1:
			((void(*)(uintptr_t))p->func)(p->arg);
			break;
#ifdef _WIN32
		case CC_STDCALL_0:
			((void(__stdcall*)(void))p->func)();
			break;
		case CC_STDCALL_1:
			((void(__stdcall*)(uintptr_t))p->func)(p->arg);
			break;
#endif
		default:
			assert(0 && "call_exit_funcs: invalid calling convention in ExitFunc!");
		}
		p++;
	}
	num_exit_funcs = 0;
}


int atexit2(void* func, uintptr_t arg, CallConvention cc)
{
	if(num_exit_funcs >= MAX_EXIT_FUNCS)
	{
		assert("atexit2: too many functions registered. increase MAX_EXIT_FUNCS");
		return -1;
	}
	ExitFunc* p = &exit_funcs[num_exit_funcs++];
	p->func = func;
	p->arg = arg;
	p->cc = cc;
	return 0;
}


int atexit2(void* func)
{
	return atexit2(func, 0, CC_CDECL_0);
}



// call from main as early as possible.
void lib_init()
{
	atexit(call_exit_funcs);
}
