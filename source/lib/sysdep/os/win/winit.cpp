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
 * windows-specific module init and shutdown mechanism
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/winit.h"

#include "lib/sysdep/os/win/win.h"	// GetTickCount for quick'n dirty timing

// see http://blogs.msdn.com/larryosterman/archive/2004/09/27/234840.aspx
// for discussion of a similar mechanism.
//
// note: this module is kept distinct from the CRT's init/shutdown mechanism
// to insulate against changes there. another advantage is that callbacks
// can return Status instead of int.

// currently (2008-02-17) the init groups are populated as follows:
//   critical : wposix
//   early    : wutil
//   early2   : whrt, wdbg_heap
//   main     : waio, wsock, wtime, wdir_watch
//   late     : wsdl

typedef Status (*PfnLibError)();

// pointers to start and end of function tables.
// notes:
// - COFF tosses out empty segments, so we have to put in one value
//   (zero, because CallFunctionPointers has to ignore entries =0 anyway).
// - ASCII '$' and 'Z' come before resp. after '0'..'9', so use that to
//   bound the section names.
__declspec(allocate(".WINIT$I$")) PfnLibError initBegin = 0;
__declspec(allocate(".WINIT$IZ")) PfnLibError initEnd = 0;
__declspec(allocate(".WINIT$S$")) PfnLibError shutdownBegin = 0;
__declspec(allocate(".WINIT$SZ")) PfnLibError shutdownEnd = 0;
// note: #pragma comment(linker, "/include") is not necessary since
// these are referenced below.


/**
 * call into a range of function pointers.
 * @param [begin, end): STL-style range
 *
 * note: pointers = 0 are ignored. this is because the above placeholders
 * are initialized to 0 and because the range may be larger than
 * expected due to COFF section padding (with zeroes).
 **/
static void CallFunctionPointers(PfnLibError* begin, PfnLibError* end)
{
	const DWORD t0 = GetTickCount();

	for(PfnLibError* ppfunc = begin; ppfunc < end; ppfunc++)
	{
		if(*ppfunc)
		{
			(*ppfunc)();
		}
	}

	const DWORD t1 = GetTickCount();
	debug_printf("WINIT| total elapsed time in callbacks %d ms (+-10)\n", t1-t0);
}


void winit_CallInitFunctions()
{
	CallFunctionPointers(&initBegin, &initEnd);
}

void winit_CallShutdownFunctions()
{
	CallFunctionPointers(&shutdownBegin, &shutdownEnd);
}
