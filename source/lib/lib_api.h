/* Copyright (c) 2010 Wildfire Games
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

#ifndef INCLUDED_LIB_API
#define INCLUDED_LIB_API

#include "lib/sysdep/compiler.h"

// note: EXTERN_C cannot be used because shared_ptr is often returned
// by value, which requires C++ linkage.

#ifdef LIB_STATIC_LINK
# define LIB_API
#else
# if MSC_VERSION
#  ifdef LIB_BUILD
#   define LIB_API __declspec(dllexport)
#  else
#   define LIB_API __declspec(dllimport)
#   ifdef NDEBUG
#    pragma comment(lib, "lowlevel.lib")
#   else
#    pragma comment(lib, "lowlevel_d.lib")
#   endif
#  endif
# elif GCC_VERSION
#  ifdef LIB_BUILD
#   define LIB_API __attribute__ ((visibility("default")))
#  else
#   define LIB_API
#  endif
# else
#  error "Don't know how to define LIB_API for this compiler"
# endif
#endif

#endif	// #ifndef INCLUDED_LIB_API
