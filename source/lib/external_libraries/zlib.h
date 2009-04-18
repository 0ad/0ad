/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : zlib.h
 * Project     : 0 A.D.
 * Description : bring in ZLib header+library, with compatibility fixes
 * =========================================================================
 */

#ifndef INCLUDED_ZLIB
#define INCLUDED_ZLIB

// zlib.h -> zconf.h includes <windows.h>, which causes conflicts.
// define the include guard to prevent it from actually being included and
// then manually define the few things that are actually needed.
#define _WINDOWS_	// windows.h include guard
#ifndef WINAPI
# define WINAPI __stdcall
# define WINAPIV __cdecl
#endif

#ifndef ZLIB_STATIC
#define ZLIB_DLL
#endif

#include <zlib.h>

// automatically link against the required library
#if MSC_VERSION
# ifdef NDEBUG
#  pragma comment(lib, "zlib1.lib")
# else
#  pragma comment(lib, "zlib1d.lib")
# endif
#endif

#endif	// #ifndef INCLUDED_ZLIB
