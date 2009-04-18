/**
 * =========================================================================
 * File        : zlib.h
 * Project     : 0 A.D.
 * Description : bring in ZLib header+library, with compatibility fixes
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
