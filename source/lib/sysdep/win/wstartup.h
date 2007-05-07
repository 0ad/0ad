/**
 * =========================================================================
 * File        : wstartup.h
 * Project     : 0 A.D.
 * Description : windows-specific entry point and startup code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WSTARTUP
#define INCLUDED_WSTARTUP

/**
 * call at the beginning of main(). exact requirements: after _cinit AND
 * before any use of sysdep/win/* or call to atexit.
 *
 * rationale:
 * our Windows-specific init code needs to run before the rest of the
 * main() code. ideally this would happen automagically.
 * one possibility is using WinMain as the entry point, and then calling the
 * application's main(), but this is expressly forbidden by the C standard.
 * VC apparently makes use of this and changes its calling convention.
 * if we call it, everything appears to work but stack traces in
 * release mode are incorrect (symbol address is off by 4).
 *
 * another alternative is re#defining the app's main function to app_main,
 * having the OS call our main, and then dispatching to app_main.
 * however, this leads to trouble when another library (e.g. SDL) wants to
 * do the same.
 *
 * moreover, this file is compiled into a static library and used both for
 * the 0ad executable as well as the separate self-test. this means
 * we can't enable the main() hook for one and disable it in the other.
 *
 * the consequence is that automatic init isn't viable. this is
 * unfortunate because integration into new projects requires
 * remembering to call the init function, but it can't be helped.
 **/
extern void wstartup_PreMainInit();

// entry points (normal and without SEH wrapper; see definition)
EXTERN_C int entry();
EXTERN_C int entry_noSEH();

#endif	// #ifndef INCLUDED_WSTARTUP
