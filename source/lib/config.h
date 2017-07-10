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
 * compile-time configuration for the entire project
 */

#ifndef INCLUDED_CONFIG
#define INCLUDED_CONFIG

// notes:
// - this file is included in the PCH and thus affects the entire project.
//   to avoid unnecessary full rebuilds, place settings of more limited
//   applicability in config2.h and explicitly include that header.
// - config macros are always defined; their values (1 or 0) are tested
//   with #if instead of #ifdef. this protects against typos by at least
//   causing a warning if the tested macro is undefined.
// - allow override via compiler settings by checking #ifndef.

// precompiled headers
#ifndef CONFIG_ENABLE_PCH
# define CONFIG_ENABLE_PCH 1	// improve build performance
#endif

// frame pointers
#ifndef CONFIG_OMIT_FP
# ifdef NDEBUG
#  define CONFIG_OMIT_FP 1	// improve performance
# else
#  define CONFIG_OMIT_FP 0	// enable use of ia32's fast stack walk
# endif
#endif

// try to prevent any exceptions from being thrown - even by the C++
// standard library. useful only for performance tests.
#ifndef CONFIG_DISABLE_EXCEPTIONS
# define CONFIG_DISABLE_EXCEPTIONS 0
#endif

// enable additional debug checks (potentially rather slow).
#ifndef CONFIG_ENABLE_CHECKS
# define CONFIG_ENABLE_CHECKS 0
#endif

// static type checking with Dehydra
#ifndef CONFIG_DEHYDRA
# define CONFIG_DEHYDRA 0
#endif

// allow the use of Boost? (affects PCH and several individual modules)
#ifndef CONFIG_ENABLE_BOOST
# define CONFIG_ENABLE_BOOST 1
#endif

#endif	// #ifndef INCLUDED_CONFIG
