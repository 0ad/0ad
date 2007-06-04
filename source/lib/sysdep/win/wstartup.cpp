/**
 * =========================================================================
 * File        : wstartup.cpp
 * Project     : 0 A.D.
 * Description : windows-specific startup code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wstartup.h"

#include "winit.h"


//-----------------------------------------------------------------------------
// shutdown

// note: the alternative of using atexit has two disadvantages.
// - the call to atexit must come after _cinit, which means we'd need to be
//   called manually from main (see discussion on init below)
// - other calls to atexit from ctors or hidden compiler-generated init code
//   for static objects would cause those handlers to be called after ours,
//   which may cause shutdown order bugs.

//-----------------------------------------------------------------------------
// init

// the init functions need to be called before any use of Windows-specific
// code. in particular, static ctors may use whrt or wpthread, so we ought to
// be initialized before them as well.
//
// one possibility is using WinMain as the entry point, and then calling the
// application's main(), but this is expressly forbidden by the C standard.
// VC apparently makes use of this and changes its calling convention.
// if we call it, everything appears to work but stack traces in
// release mode are incorrect (symbol address is off by 4).
//
// another alternative is re#defining the app's main function to app_main,
// having the OS call our main, and then dispatching to app_main.
// however, this leads to trouble when another library (e.g. SDL) wants to
// do the same.
// moreover, this file is compiled into a static library and used both for
// the 0ad executable as well as the separate self-test. this means
// we can't enable the main() hook for one and disable it in the other.
//
// requiring users to call us at the beginning of main is brittle in general,
// comes after static ctors, and is difficult to achieve in external code
// such as the (automatically generated) self-test.
//
// commandeering the entry point, doing init there and then calling
// mainCRTStartup would work, but doesn't help with shutdown - additional
// measures are required (see above). note that this approach means we're
// initialized before _cinit, denying the use of non-stateless CRT functions.


EXTERN_C void InitAndRegisterShutdown()
{
	winit_CallInitFunctions();
	atexit(winit_CallShutdownFunctions);
}

#pragma data_seg(".CRT$XCB")
EXTERN_C void(*pInitAndRegisterShutdown)() = InitAndRegisterShutdown;
#pragma comment(linker, "/include:_pInitAndRegisterShutdown")
#pragma data_seg()


//-----------------------------------------------------------------------------
// SEH wrapper

#include "wdbg.h"		// wdbg_exception_filter

typedef int(*PfnIntVoid)(void);

static int RunWithinTryBlock(PfnIntVoid func)
{
	int ret;
	//__try
	{
		ret = func();
	}
	//__except(wdbg_exception_filter(GetExceptionInformation()))
	{
		ret = -1;
	}
	return ret;
}
