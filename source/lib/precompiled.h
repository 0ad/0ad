/**
 * =========================================================================
 * File        : precompiled.h
 * Project     : 0 A.D.
 * Description : precompiled header. must be the first non-comment part
 *             : of every source file (VC6..8 requirement).
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#define _SECURE_SCL 0

#include "lib/sysdep/compiler.h"	// MSC_VERSION, HAVE_PCH

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
# if ICC_VERSION
#  pragma warning(disable:1786)	// function is deprecated (disabling 4996 isn't sufficient)
#  pragma warning(disable:1684)	// conversion from pointer to same-sized integral type
# endif
#endif


//
// headers made available everywhere for convenience
//

#include "lib/sysdep/os.h"
#include "lib/sysdep/stl.h"
#include "lib/sysdep/arch.h"

#include "lib/lib_api.h"

#include "lib/types.h"
#include "lib/lib.h"
#include "lib/lib_errors.h"
#include "lib/secure_crt.h"
#include "lib/debug.h"
#include "lib/code_annotation.h"

// Boost
// .. if this package isn't going to be statically linked, we're better off
// using Boost via DLL. (otherwise, we would have to ensure the exact same
// compiler is used, which is a pain because MSC8, MSC9 and ICC 10 are in use)
#ifndef LIB_STATIC_LINK
# define BOOST_ALL_DYN_LINK 
#endif
#include <boost/utility.hpp>	// noncopyable
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
using boost::shared_ptr;	// has been added to TR1
#include "lib/external_libraries/boost_filesystem.h"

// (this must come after boost and common lib headers)
#include "lib/posix/posix.h"


//
// precompiled headers
//

// if PCHs are supported and enabled, we make an effort to include all
// system headers. otherwise, only a few central headers (e.g. types)
// are pulled in and source files must include all the system headers
// they use. this policy ensures good compile performance whether or not
// PCHs are being used.

#include "lib/config.h"	// CONFIG_ENABLE_PCH
#include "lib/sysdep/compiler.h"	// HAVE_PCH

#if CONFIG_ENABLE_PCH && HAVE_PCH

// anything placed here won't need to be compiled in each translation unit,
// but will cause a complete rebuild if they change.

// all new-form C library headers
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
//#include <ciso646> // defines e.g. "and" to "&". unnecessary and causes trouble with asm.
#include <climits>
#include <clocale>
#include <cmath>
//#include <csetjmp>	// incompatible with libpng on Debian/Ubuntu
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

#endif // #if CONFIG_PCH

// restore temporarily-disabled warnings
#if MSC_VERSION
# pragma warning(default:4702)
#endif
