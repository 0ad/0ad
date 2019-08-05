/* Copyright (C) 2019 Wildfire Games.
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

#include "lib/config.h"	            // CONFIG_ENABLE_BOOST, CONFIG_ENABLE_PCH
#include "lib/sysdep/compiler.h"    // MSC_VERSION, HAVE_PCH

// must come before any STL headers are included
#if MSC_VERSION
# ifdef NDEBUG	// release: disable all checks
#  define _HAS_ITERATOR_DEBUGGING 0
#  define _SECURE_SCL 0
# endif
#endif

// disable some common and annoying warnings
// (as soon as possible so that headers below are covered)
#include "lib/pch/pch_warnings.h"

#if ICC_VERSION
#include <mathimf.h>	// (must come before <cmath> or <math.h> (replaces them))
double __cdecl abs(double x);	// not declared by mathimf
#endif


//
// headers made available everywhere for convenience
//

#include "lib/posix/posix_types.h"	// (must come before any system headers because it fixes off_t)
#include "lib/code_annotation.h"
#include "lib/sysdep/arch.h"
#include "lib/sysdep/os.h"
#include "lib/sysdep/stl.h"
#include "lib/lib_api.h"
#include "lib/types.h"
#include "lib/debug.h"
#include "lib/lib.h"
#include "lib/secure_crt.h"

#if CONFIG_ENABLE_BOOST
# include "lib/pch/pch_boost.h"
#endif

#include <array>
#include <memory>
using std::shared_ptr;

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

// These two files are included (directly or indirectly) by about half of the .cpp files.
// Changing these thus forces recompilation of most of the project regardless,
// and putting them in the precompiled header cuts a large amount of time even even on incremental builds.
#include "ps/CLogger.h"
#include "ps/Profile.h"

#endif // #if CONFIG_ENABLE_PCH && HAVE_PCH
