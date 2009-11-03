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

#include "posix_types.h"
#include "posix_aio.h"
#include "posix_dlfcn.h"
#include "posix_filesystem.h"
#include "posix_mman.h"
#include "posix_pthread.h"
//#include "posix_sock.h"
//#include "posix_terminal.h"
//#include "posix_time.h"
#include "posix_utsname.h"


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
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define wcscasecmp wcsicmp
#define wcsncasecmp wcsnicmp
#endif

#if OS_MACOSX
# define EMULATE_WCSDUP 1
#else
# define EMULATE_WCSDUP 0
#endif

#if EMULATE_WCSDUP
extern wchar_t* wcsdup(const wchar_t* str);
#endif


// rint*, fminf, fpclassify (too few/diverse to make separate HAVE_ for each)
#if HAVE_C99 || GCC_VERSION
# define HAVE_C99_MATH 1
#else
# define HAVE_C99_MATH 0
#endif

#if !HAVE_C99_MATH
// round float to nearest integral value, according to
// current rounding mode.
extern float rintf(float f);
extern double rint(double d);
// return minimum/maximum of two floats.
extern float fminf(float a, float b);
extern float fmaxf(float a, float b);

extern size_t fpclassifyf(float f);
extern size_t fpclassifyd(double d);
#define fpclassify(x) ( (sizeof(x) == sizeof(float))? fpclassifyf(x) : fpclassifyd(x) )

// these definitions "happen" to match IA32_FP_* and allow using
// ia32_fp_classify without having to translate the return value.
// we don't #define to IA32_FP_* to avoid dependency.
#  define FP_NAN       0x0100
#  define FP_NORMAL    0x0400
#  define FP_INFINITE  (FP_NAN | FP_NORMAL)
#  define FP_ZERO      0x4000
#  define FP_SUBNORMAL (FP_NORMAL | FP_ZERO)

# define isnan(d) (fpclassify(d) == FP_NAN)
# define isfinite(d) ((fpclassify(d) & (FP_NORMAL|FP_ZERO)) != 0)
# define isinf(d) (fpclassify(d) == FP_NAN|FP_NORMAL)
# define isnormal(d) (fpclassify(d) == FP_NORMAL)
//# define signbit
#else	// HAVE_C99_MATH
// Some systems have C99 support but in C++ they provide only std::isfinite
// and not isfinite. C99 specifies that isfinite is a macro, so we can use
// #ifndef and define it if it's not there already.
// We've included <cmath> above to make sure it defines that macro.
# ifndef isfinite
#  define isfinite std::isfinite
#  define isnan std::isnan
#  define isinf std::isinf
#  define isnormal std::isnormal
# endif
#endif	// HAVE_C99_MATH

#endif	// #ifndef INCLUDED_POSIX
