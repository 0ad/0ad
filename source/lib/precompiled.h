// precompiled header. must be the first non-comment part of every source
// file (VC6/7 requirement).
//
// if the compiler supports PCH (i.e. HAVE_PCH is defined), this 
// tries to include all headers that may be needed. otherwise, all source
// files will still need to include this (for various global fixes and the
// memory trackers), but additionally include all required headers.
//
// this policy yields the best compile performance with or without PCH.

#ifdef _MSC_VER
#pragma warning(disable:4996)	// function is deprecated
#pragma warning(disable:4786)	// identifier truncated to 255 chars
// VS2005 code analysis warnings - disable the very frequent ones:
#pragma warning(disable:6011)	// dereferencing NULL pointer
#pragma warning(disable:6246)	// local declaration hides declaration of the same name in outer scope
#endif

// make these available everywhere for convenience:
#include "lib/config.h"
#include "lib/types.h"
#include "lib/string_s.h"	// CRT secure string
#include "lib/debug.h"
#include "ps/Pyrogenesis.h"	// MICROLOG and old error system


//
// memory headers
//

// these are all system headers that contain "new", "malloc" etc.; they must
// come before the memory tracker headers to avoid conflicts with their
// macros. therefore, they are always included, even if !HAVE_PCH.

#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <malloc.h>
#include <new>
#include <memory>
#include <cstring>	// uses placement new
#include <string>


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

#ifdef HAVE_PCH

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
#include <csetjmp>
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
#include <strstream>
#include <typeinfo>
#include <valarray>

// STL extensions
#ifdef __GNUC__
# include <ext/hash_map>
# include <ext/hash_set>
#else
# include <hash_map>
# include <hash_set>
#endif


// CStr is included very frequently, so a reasonable amount of time is saved
// by including it here. (~10% in a full rebuild, as of r2365)
#include "ps/CStr.h"

// Some other external libraries that are used in several places:
#include "scripting/SpiderMonkey.h"
#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"

// (further headers to be precompiled go here)

#endif // #ifdef HAVE_PCH


//
// memory trackers
//

// these must be included from every file to make sure all allocations
// are hooked. placing in the precompiled header is more convenient than
// manually #including from every file, but requires that all system
// headers containing "new", "malloc" etc. come before this (see above).

// use custom memory tracker (lib/mmgr.cpp)
#if defined(CONFIG_USE_MMGR)
# include "mmgr.h"
// use VC debug heap (superceded by mmgr; it remains for completeness)
#elif defined(HAVE_VC_DEBUG_ALLOC)
  // can't define _CRTDBG_MAP_ALLOC because crtdbg.h has a broken 'new',
  // so manually redefine the appropriate functions.
# define new           new(_NORMAL_BLOCK, __FILE__, __LINE__)
# define malloc(s)     _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
# define calloc(c, s)  _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
# define realloc(p, s) _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
# define free(p)       _free_dbg(p, _NORMAL_BLOCK)
#endif
