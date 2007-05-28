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
// notes:
// - COFF tosses out empty segments, so we have to put in one value
//   (zero, because CallFunctionPointers has to ignore entries =0 anyway).
// - ASCII '$' and 'Z' come before resp. after '0'..'9', so use that to
//   bound the section names.
#pragma SECTION_INIT($)
PfnLibErrorVoid initBegin = 0;
#pragma SECTION_INIT(Z)
PfnLibErrorVoid initEnd = 0;
#pragma SECTION_SHUTDOWN($)
PfnLibErrorVoid shutdownBegin = 0;
#pragma SECTION_SHUTDOWN(Z)
PfnLibErrorVoid shutdownEnd = 0;
#pragma SECTION_RESTORE
// note: /include is not necessary, since these are referenced below.

#pragma comment(linker, "/merge:.WINIT=.data")


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


void winit_CallInitFunctions()
{
	CallFunctionPointers(&initBegin, &initEnd);
}

void winit_CallShutdownFunctions()
{
	CallFunctionPointers(&shutdownBegin, &shutdownEnd);
}
