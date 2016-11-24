/* Copyright (c) 2015 Wildfire Games
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
 * compiler-specific macros and fixes
 */

#ifndef INCLUDED_COMPILER
#define INCLUDED_COMPILER

// detect compiler and its version (0 if not present, otherwise
// major*100 + minor). note that more than one *_VERSION may be
// non-zero due to interoperability (e.g. ICC with MSC).
// .. VC
#ifdef _MSC_VER
# define MSC_VERSION _MSC_VER
#else
# define MSC_VERSION 0
#endif
// .. ICC (VC-compatible, GCC-compatible)
#if defined(__INTEL_COMPILER)
# define ICC_VERSION __INTEL_COMPILER
#else
# define ICC_VERSION 0
#endif
// .. LCC (VC-compatible)
#if defined(__LCC__)
# define LCC_VERSION __LCC__
#else
# define LCC_VERSION 0
#endif
// .. GCC
#ifdef __GNUC__
# define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__)
#else
# define GCC_VERSION 0
#endif
// .. Clang/LLVM (GCC-compatible)
// use Clang's feature checking macros to check for availability of features
// http://clang.llvm.org/docs/LanguageExtensions.html#feature-checking-macros
#ifdef __clang__
# define CLANG_VERSION (__clang_major__*100 + __clang_minor__)
#else
# define CLANG_VERSION 0
#endif

// Clang/LLVM feature check macro compatibility
#ifndef __has_feature
# define __has_feature(x) 0
#endif


// are PreCompiled Headers supported?
#if MSC_VERSION
# define HAVE_PCH 1
#elif defined(USING_PCH)
# define HAVE_PCH 1
#else
# define HAVE_PCH 0
#endif


// check if compiling in pure C mode (not C++) with support for C99.
// (this is more convenient than testing __STDC_VERSION__ directly)
//
// note: C99 provides several useful but disjunct bits of functionality.
// unfortunately, most C++ compilers do not offer a complete implementation.
// however, many of these features are likely to be added to C++, and/or are
// already available as extensions. what we'll do is add a HAVE_ macro for
// each feature and test those instead. they are set if HAVE_C99, or also if
// the compiler happens to support something compatible.
//
// rationale: lying about __STDC_VERSION__ via Premake so as to enable support
// for some C99 functions doesn't work. Mac OS X headers would then use the
// restrict keyword, which is never supported by g++ (because that might
// end up breaking valid C++98 programs).
#define HAVE_C99 0
#ifdef __STDC_VERSION__
# if __STDC_VERSION__ >= 199901L
#  undef  HAVE_C99
#  define HAVE_C99 1
# endif
#endif

// Streaming SIMD Extensions (not supported by all GCC)
// this only ascertains compiler support; use x86_x64::Cap to
// check whether the instructions are supported by the CPU.
#ifndef HAVE_SSE
# if GCC_VERSION && defined(__SSE__)
#  define HAVE_SSE 1
# elif MSC_VERSION	// also includes ICC
#  define HAVE_SSE 1
# else
#  define HAVE_SSE 0
# endif
#endif

#ifndef HAVE_SSE2
# if GCC_VERSION && defined(__SSE2__)
#  define HAVE_SSE2 1
# elif MSC_VERSION	// also includes ICC
#  define HAVE_SSE2 1
# else
#  define HAVE_SSE2 0
# endif
#endif

#endif	// #ifndef INCLUDED_COMPILER
