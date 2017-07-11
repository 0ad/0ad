/* Copyright (C) 2010 Wildfire Games.
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
 * bring in ZLib header+library, with compatibility fixes
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
