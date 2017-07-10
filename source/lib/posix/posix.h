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
 * definitions for a subset of POSIX.
 */

/*

[KEEP IN SYNC WITH WIKI]

this header makes available commonly used POSIX (Portable Operating System
Interface) definitions, e.g. thread, file I/O and socket APIs.
on Linux and OS X we just include the requisite headers; Win32 doesn't really
support POSIX (*), so we have to implement everything ourselves.

rationale: this is preferable to a wrapper for several reasons:
- less code (implementation is only needed on Win32)
- no lock-in (the abstraction may prevent not-designed-for operations
  that the POSIX interface would have allowed)
- familiarity (many coders already know POSIX)

if a useful definition is missing, feel free to add it!

implementation reference is the "Single Unix Specification v3"
(http://www.unix.org/online.html) - it's similar to the POSIX standard
(superset?) and freely available.


* Win32 does have a POSIX subsystem (mandated by a government contract),
but it is crippled. only apps with the PE header 'subsystem' field set to
"POSIX" can use the appendant DLL, and then they can't call the regular
Windows APIs. this is obviously unacceptable - GDI is needed to set up OpenGL.

we therefore need to emulate POSIX functions using the Win32 API.
fortunately, many POSIX functions are already implemented in the VC CRT and
need only be renamed (e.g. _open, _stat).

*/

#ifndef INCLUDED_POSIX
#define INCLUDED_POSIX

#include <cmath>	// see isfinite comment below

#if OS_WIN
# include "lib/sysdep/os/win/wposix/wposix.h"
#endif

#include "lib/posix/posix_types.h"

// disabled to reduce dependencies. include them where needed.
//#include "lib/posix/posix_aio.h"
//#include "lib/posix/posix_dlfcn.h"
//#include "lib/posix/posix_filesystem.h"
//#include "lib/posix/posix_mman.h"
//#include "lib/posix/posix_pthread.h"
//#include "lib/posix/posix_time.h"
//#include "lib/posix/posix_utsname.h"


// note: the following need only be #defined (instead of defining a
// trampoline function) because the redefined functions are already
// declared by standard headers.

// provide C99 *snprintf functions if compiler doesn't already
// (MinGW does, VC7.1 doesn't).
#if MSC_VERSION
# define snprintf _snprintf
# define swprintf _snwprintf
# define vsnprintf _vsnprintf
# define vswprintf _vsnwprintf
#endif

// VC doesn't define str[n]casecmp
#if MSC_VERSION
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define wcscasecmp _wcsicmp
#define wcsncasecmp _wcsnicmp
#endif

#if OS_MACOSX
# define EMULATE_WCSDUP 1
# define EMULATE_WCSCASECMP 1
#else
# define EMULATE_WCSDUP 0
# define EMULATE_WCSCASECMP 0
#endif

#if EMULATE_WCSDUP
extern wchar_t* wcsdup(const wchar_t* str);
#endif

#if EMULATE_WCSCASECMP
extern int wcscasecmp(const wchar_t* s1, const wchar_t* s2);
#endif

// Some systems have C99 support but in C++ they provide only std::isfinite
// and not isfinite. C99 specifies that isfinite is a macro, so we can use
// #ifndef and define it if it's not there already.
// We've included <cmath> above to make sure it defines that macro.
#ifndef isfinite
# if MSC_VERSION
#  define isfinite _finite
#  define isnan _isnan
# else
#  define isfinite std::isfinite
#  define isnan std::isnan
# endif
#endif

#endif	// #ifndef INCLUDED_POSIX
