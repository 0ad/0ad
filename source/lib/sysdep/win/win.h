// compatibility fixes when compiling on Win32
// Copyright (c) 2002-2005 Jan Wassenberg
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

#ifndef WIN_H__
#define WIN_H__

#ifndef _WIN32
#error "including win.h without _WIN32 defined"
#endif

#include <wchar.h>

// C99
#define snprintf _snprintf
#define swprintf _snwprintf
#define vsnprintf _vsnprintf
#define vsnwprintf _vsnwprintf

#include <stddef.h>	// wchar_t

// libpng.h -> zlib.h -> zconf.h includes <windows.h>, which causes conflicts.
// prevent that, and define what they actually need from windows.h.
// incidentally, this requires all dependents of windows.h to include
// sysdep/win/win_internal.h instead.
#define _WINDOWS_	// windows.h include guard
#define WINAPI __stdcall
#define WINAPIV __cdecl

#endif	// #ifndef WIN_H__
