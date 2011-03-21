#ifndef INCLUDED_PCH_STDLIB
#define INCLUDED_PCH_STDLIB

#if !MINIMAL_PCH
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
#endif // !MINIMAL_PCH

#if MINIMAL_PCH < 2
// common C++98 STL headers
#include <algorithm>
#include <vector>
#endif

#if MINIMAL_PCH < 3
// all other C++98 STL headers
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
#endif

#if !MINIMAL_PCH
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
#endif // !MINIMAL_PCH

#if !MINIMAL_PCH
// STL extensions
#if GCC_VERSION >= 402 // (see comment in stl.h about GCC versions)
# include <tr1/unordered_map>
# include <tr1/unordered_set>
#elif GCC_VERSION
# include <ext/hash_map>
# include <ext/hash_set>
#else
# include <hash_map>
# include <hash_set>
#endif
#endif // !MINIMAL_PCH

#endif	// #ifndef INCLUDED_PCH_STDLIB
