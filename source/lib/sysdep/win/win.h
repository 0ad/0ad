/**
 * =========================================================================
 * File        : win.h
 * Project     : 0 A.D.
 * Description : various compatibility fixes for Win32.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef WIN_H__
#define WIN_H__

#include "lib/config.h"

#if !OS_WIN
#error "win.h: do not include if not compiling for Windows"
#endif

#include <wchar.h>

// provide C99 *snprintf functions if compiler doesn't already
// (MinGW does, VC7.1 doesn't).
#if !HAVE_NPRINTF
# define snprintf _snprintf
# define swprintf _snwprintf
# define vsnprintf _vsnprintf
# define vswprintf _vsnwprintf
#endif

#include <stddef.h>	// wchar_t

// libpng.h -> zlib.h -> zconf.h includes <windows.h>, which causes conflicts.
// prevent that, and define what they actually need from windows.h.
// incidentally, this requires all dependents of windows.h to include
// sysdep/win/win_internal.h instead.
#define _WINDOWS_	// windows.h include guard
#define WINAPI __stdcall
#define WINAPIV __cdecl


// rationale for manual init:
// our Windows-specific init code needs to run before the regular main() code.
// ideally this would happen automagically.
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
//
// moreover, this file is compiled into a static library and used both for
// the 0ad executable as well as the separate self-test. this means
// we can't enable the main() hook for one and disable in the other.
//
// the consequence is that automatic init isn't viable. users MUST call this
// at the beginning of main (or at least before using any lib function).
// this is unfortunate because integration into new projects requires
// remembering to call the init function, but it can't be helped.
extern void win_pre_main_init();

#endif	// #ifndef WIN_H__
