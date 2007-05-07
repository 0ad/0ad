/**
 * =========================================================================
 * File        : wstartup.cpp
 * Project     : 0 A.D.
 * Description : windows-specific entry point and startup code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wstartup.h"

#include "winit.h"
#include "wdbg.h"			// wdbg_exception_filter
#include "win_internal.h"	// GetExceptionInformation

#if MSC_VERSION >= 1400
#include <process.h>		// __security_init_cookie
#define NEED_COOKIE_INIT
#endif

// this module is responsible for startup and triggering winit's calls to
// registered functions at the appropriate times. control flow overview:
// entry [-> RunWithinTryBlock] -> InitAndCallMain -> MainCRTStartup ->
// main -> wstartup_PreMainInit.
// our atexit handler is called as the last of them, provided constructors
// do not use atexit! (this requirement is documented)
//
// rationale: see declaration of wstartup_PreMainInit.


void wstartup_PreMainInit()
{
	winit_CallPreMainFunctions();

	atexit(winit_CallShutdownFunctions);

	// no point redirecting stdout yet - the current directory
	// may be incorrect (file_set_root not yet called).
	// (w)sdl will take care of it anyway.
}


// these aren't defined in VC include files, so we have to do it manually.
#ifdef USE_WINMAIN
EXTERN_C int WinMainCRTStartup(void);
#else
EXTERN_C int mainCRTStartup(void);
#endif

// (moved into a separate function because it's used by both
// entry and entry_noSEH)
static int InitAndCallMain()
{
	// perform all initialization that needs to run before _cinit
	// (i.e. when C++ ctors are called).
	// be very careful to avoid non-stateless libc functions!
	winit_CallPreLibcFunctions();

	int ret;
#ifdef USE_WINMAIN
	ret = WinMainCRTStartup(); // calls _cinit and then our WinMain
#else
	ret = mainCRTStartup();    // calls _cinit and then our main
#endif
	return ret;
}


typedef int(*PfnIntVoid)(void);

static int RunWithinTryBlock(PfnIntVoid func)
{
	int ret;
	__try
	{
		ret = func();
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
	return RunWithinTryBlock(InitAndCallMain);
}


// Alternative entry point, for programs that don't want the SEH handler
// (e.g. unit tests, where it's better to let the debugger handle any errors)
int entry_noSEH()
{
#ifdef NEED_COOKIE_INIT
	// see above. this is also necessary here in case pre-libc init
	// functions use SEH.
	__security_init_cookie();
#endif

	return InitAndCallMain();
}
