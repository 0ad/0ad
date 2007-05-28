/**
 * =========================================================================
 * File        : wstartup.cpp
 * Project     : 0 A.D.
 * Description : windows-specific entry point and startup code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

// this module can wrap the program in a SEH __try block and takes care of
// calling winit's functions at the appropriate times.
// to use it, set Linker Options -> Advanced -> Entry Point to
// "entry" (without quotes).
//
// besides commandeering the entry point, it hooks ExitProcess.
// control flow overview: entry [-> RunWithinTryBlock] -> InitAndCallMain ->
// (init) -> mainCRTStartup -> main -> exit -> HookedExitProcess ->
// (shutdown) -> ExitProcess.

#include "precompiled.h"
#include "wstartup.h"

#include "win.h"
#include <process.h>	// __security_init_cookie
#include <detours.h>

#include "winit.h"
#include "wdbg.h"		// wdbg_exception_filter


#if MSC_VERSION
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "detoured.lib")
#endif


//-----------------------------------------------------------------------------
// do shutdown at exit

// note: the alternative of using atexit has two disadvantages.
// - the call to atexit must come after _cinit, which means we'd need to be
//   called manually from main (see discussion on init below)
// - other calls to atexit from ctors or hidden compiler-generated init code
//   for static objects would cause those handlers to be called after ours,
//   which may cause shutdown order bugs.

static VOID (WINAPI *RealExitProcess)(UINT uExitCode);

static VOID WINAPI HookedExitProcess(UINT uExitCode)
{
	winit_CallShutdownFunctions();

	RealExitProcess(uExitCode);
}

static void InstallExitHook()
{
	// (can't do this in a static initializer because they haven't run yet!)
	RealExitProcess = ExitProcess;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)RealExitProcess, HookedExitProcess);
	DetourTransactionCommit();
}

//-----------------------------------------------------------------------------
// init

// the init functions need to be called before any use of Windows-specific
// code.
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
// requiring users to call us at the beginning of main is brittle in general
// and not possible with the self-test's auto-generated main file.
//
// the only alternative is to commandeer the entry point and do all init
// before calling mainCRTStartup. this means init will be finished before
// C++ static ctors run (allowing our APIs to be called from those ctors),
// but also denies init the use of any non-stateless CRT functions!

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
	winit_CallInitFunctions();

	InstallExitHook();

	int ret;
#ifdef USE_WINMAIN
	ret = WinMainCRTStartup(); // calls _cinit and then our WinMain
#else
	ret = mainCRTStartup();    // calls _cinit and then our main
#endif
	return ret;
}


//-----------------------------------------------------------------------------
// entry point and SEH wrapper

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
#if MSC_VERSION >= 1400
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
#if MSC_VERSION >= 1400
	// see above. this is also necessary here in case pre-libc init
	// functions use SEH.
	__security_init_cookie();
#endif

	return InitAndCallMain();
}
