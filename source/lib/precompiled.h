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

/*
 * precompiled header. must be the first non-comment part of every
 * source file (VC++ requirement).
 */

// some libraries have only a small number of source files, and the
// overhead of including loads of headers here outweighs the improvements to
// incremental rebuild performance.
// they can set MINIMAL_PCH to 1 so we include far fewer headers (but
// still do the global disabling of warnings and include config headers etc),
// or set it to 2 to remove STL headers too (precompiling STL helps performance
// in most non-tiny cases)
#ifndef MINIMAL_PCH
# define MINIMAL_PCH 0
#endif

#include "lib/config.h"	// CONFIG_ENABLE_PCH
#include "lib/sysdep/compiler.h"	// MSC_VERSION, HAVE_PCH
#include "lib/sysdep/os.h"	// (must come before posix_types.h)

// must come before any STL headers are included
#if MSC_VERSION && defined(NDEBUG)
# define _SECURE_SCL 0
#endif

// disable some common and annoying warnings
// must come after compiler.h, but as soon as possible so that
// headers below are covered
#include "lib/pch/pch_warnings.h"

#if ICC_VERSION
#include <mathimf.h>	// (must come before <cmath> or <math.h> (replaces them))
double __cdecl abs(double x);	// not declared by mathimf
long double __cdecl abs(long double x);	// required for Eigen
#endif


//
// headers made available everywhere for convenience
//

#include "lib/posix/posix_types.h"	// (must come before any system headers because it fixes off_t)
#include "lib/code_annotation.h"
#include "lib/code_generation.h"
#include "lib/sysdep/arch.h"
#include "lib/sysdep/stl.h"
#include "lib/lib_api.h"
#include "lib/types.h"
#include "lib/debug.h"
#include "lib/lib.h"
#include "lib/secure_crt.h"

#include "lib/pch/pch_boost.h"

// (must come after boost and common lib headers, but before re-enabling
// warnings to avoid boost spew)
#include "lib/posix/posix.h"	


//
// precompiled headers
//

// if PCHs are supported and enabled, we make an effort to include all
// system headers. otherwise, only a few central headers (e.g. types)
// are pulled in and source files must include all the system headers
// they use. this policy ensures good compile performance whether or not
// PCHs are being used.

#if CONFIG_ENABLE_PCH && HAVE_PCH

// anything placed here won't need to be compiled in each translation unit,
// but will cause a complete rebuild if they change.

#include "lib/pch/pch_stdlib.h"

#endif // #if CONFIG_ENABLE_PCH && HAVE_PCH

// restore temporarily-disabled warnings
#if ICC_VERSION
# pragma warning(pop)
#endif
#if MSC_VERSION
# pragma warning(default:4702)
#endif
