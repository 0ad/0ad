/* Copyright (c) 2013 Wildfire Games
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
 * fixes for various STL implementations
 */

#ifndef INCLUDED_STL
#define INCLUDED_STL

#include "lib/config.h"
#include "compiler.h"
#include <cstdlib> // indirectly pull in bits/c++config.h on Linux, so __GLIBCXX__ is defined

// detect STL version
// .. Dinkumware
#if MSC_VERSION
# include <yvals.h>	// defines _CPPLIB_VER
#endif
#if defined(_CPPLIB_VER)
# define STL_DINKUMWARE _CPPLIB_VER
#else
# define STL_DINKUMWARE 0
#endif
// .. GCC
#if defined(__GLIBCPP__)
# define STL_GCC __GLIBCPP__
#elif defined(__GLIBCXX__)
# define STL_GCC __GLIBCXX__
#else
# define STL_GCC 0
#endif
// .. ICC
#if defined(__INTEL_CXXLIB_ICC)
# define STL_ICC __INTEL_CXXLIB_ICC
#else
# define STL_ICC 0
#endif


// disable (slow!) iterator checks in release builds (unless someone already defined this)
#if STL_DINKUMWARE && defined(NDEBUG) && !defined(_SECURE_SCL)
# define _SECURE_SCL 0
#endif


// pass "disable exceptions" setting on to the STL
#if CONFIG_DISABLE_EXCEPTIONS
# if STL_DINKUMWARE
#  define _HAS_EXCEPTIONS 0
# else
#  define STL_NO_EXCEPTIONS
# endif
#endif


// OS X - fix some stream template instantiations that break 10.5 compatibility on newer SDKs
#if OS_MACOSX
# include "os/osx/osx_stl_fixes.h"
#endif

#endif	// #ifndef INCLUDED_STL
