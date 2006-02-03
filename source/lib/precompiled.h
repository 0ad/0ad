// precompiled header. must be the first non-comment part of every source
// file (VC6/7 requirement).
//
// if the compiler supports PCH (i.e. HAVE_PCH is defined), this
// tries to include all headers that may be needed. otherwise, all source
// files will still need to include this (for various global fixes and the
// memory trackers), but additionally include all required headers.
//
// this policy yields the best compile performance with or without PCH.

// must come before warning disables.
#include "lib/config.h"

// disable some common and annoying warnings
// (done as soon as possible so that headers below are covered)
#if MSC_VERSION
// .. temporarily disabled W4 (unimportant but ought to be fixed)
# pragma warning(disable:4201)	// nameless struct (Matrix3D)
# pragma warning(disable:4244)	// conversion from uintN to uint8
// .. permanently disabled W4
# pragma warning(disable:4127)	// conditional expression is constant; rationale: see STMT in lib.h.
# pragma warning(disable:4996)	// function is deprecated
# pragma warning(disable:4786)	// identifier truncated to 255 chars
# pragma warning(disable:4351)	// yes, default init of array entries is desired
// .. disabled only for the precompiled headers
# pragma warning(disable:4702)	// unreachable code (frequent in STL)
// .. VS2005 code analysis (very frequent ones)
# if MSC_VERSION >= 1400
#  pragma warning(disable:6011)	// dereferencing NULL pointer
#  pragma warning(disable:6246)	// local declaration hides declaration of the same name in outer scope
# endif
#endif

//
// headers made available everywhere for convenience
//

// note: must not include 
#include "lib/types.h"
#include "lib/string_s.h"	// CRT secure string
#include "lib/self_test.h"
#include "lib/debug.h"
#include "ps/Pyrogenesis.h"	// MICROLOG and old error system
#include <assert.h> // assert()

//
// memory headers
//

// these are all system headers that contain "new", "malloc" etc.; they must
// come before the memory tracker headers to avoid conflicts with their
// macros. therefore, they are always included, even if !HAVE_PCH.

#if OS_WIN
# include <malloc.h>
#endif

#include <new>
#include <memory>
#include <valarray>	// free() member function


//
// headers to be precompiled
//

// candidates are all system headers we may possibly need or large/rarely
// changed project headers; everything placed in here will not need to be
// compiled every time. however, if they change, the project will have to be
// completely rebuilt. (slow!)
//
// if the compiler doesn't support precompiled headers (i.e. !HAVE_PCH),
// including anything here would actually slow things down, because we might
// not otherwise need some of these headers. therefore, do nothing and rely
// on all source files (additionally) including everything they need.

#if HAVE_PCH

// all new-form C library headers
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
//#include <ciso646>
	// defines e.g. "and" to "&". unnecessary and causes trouble with asm.
#include <climits>
#include <clocale>
#include <cmath>
// Including setjmp.h here causes incompatibilities with libpng on Debian/Ubuntu
//#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <cwctype>

// all C++98 STL headers
#include <algorithm>
#include <deque>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <stack>
#include <utility>
#include <vector>

// all other C++98 headers
#include <bitset>
#include <complex>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <limits>
#include <locale>
#include <new>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <sstream>
#include <typeinfo>
#include <valarray>

// STL extensions
#if GCC_VERSION
# include <ext/hash_map>
# include <ext/hash_set>
#else
# include <hash_map>
# include <hash_set>
#endif

// some other external libraries that are used in several places:
// .. CStr is included very frequently, so a reasonable amount of time is
//    saved by including it here. (~10% in a full rebuild, as of r2365)
#include "ps/CStr.h"
#include "scripting/SpiderMonkey.h"
#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"

// (further headers to be precompiled go here)

#endif // #if HAVE_PCH

// restore temporarily-disabled warnings
#if MSC_VERSION
# pragma warning(default:4702)
#endif


//
// memory trackers
//

// these must be included from every file to make sure all allocations
// are hooked. placing in the precompiled header is more convenient than
// manually #including from every file, but requires that all system
// headers containing "new", "malloc" etc. come before this (see above).
//
// note: mmgr.h activates mmgr or the VC debug heap or nothing depending
// on CONFIG_USE_MMGR and HAVE_VC_DEBUG_ALLOC settings.
# include "mmgr.h"
