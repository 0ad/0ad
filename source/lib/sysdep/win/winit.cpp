/**
 * =========================================================================
 * File        : winit.cpp
 * Project     : 0 A.D.
 * Description : windows-specific module init and shutdown mechanism
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "winit.h"


typedef LibError (*PfnLibErrorVoid)(void);

// pointers to start and end of function tables.
// note: COFF tosses out empty segments, so we have to put in one value
// (zero, because CallFunctionPointers has to ignore entries =0 anyway).
#pragma SECTION_PRE_LIBC(A)
PfnLibErrorVoid pre_libc_begin = 0;
#pragma SECTION_PRE_LIBC(Z)
PfnLibErrorVoid pre_libc_end = 0;
#pragma SECTION_PRE_MAIN(A)
PfnLibErrorVoid pre_main_begin = 0;
#pragma SECTION_PRE_MAIN(Z)
PfnLibErrorVoid pre_main_end = 0;
#pragma SECTION_POST_ATEXIT(A)
PfnLibErrorVoid shutdown_begin = 0;
#pragma SECTION_POST_ATEXIT(Z)
PfnLibErrorVoid shutdown_end = 0;
#pragma SECTION_RESTORE
// note: /include is not necessary, since these are referenced below.

#pragma comment(linker, "/merge:.LIB=.data")


/**
 * call into a range of function pointers.
 * @param [begin, end): STL-style range
 *
 * note: pointers = 0 are ignored. this is because the above placeholders
 * are initialized to 0 and because the range may be larger than
 * expected due to COFF section padding (with zeroes).
 **/
static void CallFunctionPointers(PfnLibErrorVoid* begin, PfnLibErrorVoid* end)
{
	for(PfnLibErrorVoid* ppfunc = begin; ppfunc < end; ppfunc++)
	{
		if(*ppfunc)
			(*ppfunc)();
	}
}


void winit_CallPreLibcFunctions()
{
	CallFunctionPointers(&pre_libc_begin, &pre_libc_end);
}

void winit_CallPreMainFunctions()
{
	CallFunctionPointers(&pre_main_begin, &pre_main_end);
}

void winit_CallShutdownFunctions()
{
	CallFunctionPointers(&shutdown_begin, &shutdown_end);
}
