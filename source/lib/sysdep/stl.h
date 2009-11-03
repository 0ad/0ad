/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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


// STL_HASH_MAP, STL_HASH_MULTIMAP, STL_HASH_SET
// these containers are useful but not part of C++98. most STL vendors
// provide them in some form; we hide their differences behind macros.

#if STL_GCC

// std::tr1::unordered_* is preferred, but that causes a linker error in
// old versions of GCC, so fall back to the (deprecated) non-standard
// ext in older than 4.2.0
#if GCC_VERSION >= 402
# include <tr1/unordered_map>
# include <tr1/unordered_set>

# define STL_HASH_MAP std::tr1::unordered_map
# define STL_HASH_MULTIMAP std::tr1::unordered_multimap
# define STL_HASH_SET std::tr1::unordered_set
# define STL_HASH_MULTISET std::tr1::unordered_multiset
# define STL_SLIST __gnu_cxx::slist

template<typename T>
size_t STL_HASH_VALUE(T v)
{
	return std::tr1::hash<T>()(v);
}

#else // For older versions of GCC:

# include <ext/hash_map>
# include <ext/hash_set>

# define STL_HASH_MAP __gnu_cxx::hash_map
# define STL_HASH_MULTIMAP __gnu_cxx::hash_multimap
# define STL_HASH_SET __gnu_cxx::hash_set
# define STL_HASH_MULTISET __gnu_cxx::hash_multiset
# define STL_SLIST __gnu_cxx::slist

template<typename T>
size_t STL_HASH_VALUE(T v)
{
	return __gnu_cxx::hash<T>()(v);
}

namespace __gnu_cxx
{

// Define some hash functions that GCC doesn't provide itself:

template<> struct hash<std::wstring>
{
	size_t operator()(const std::wstring& s) const
	{
		const wchar_t* __s = s.c_str();
		unsigned long __h = 0;
		for ( ; *__s; ++__s)
			__h = 5*__h + *__s;
		return size_t(__h);
	}
};

template<> struct hash<std::string>
{
	size_t operator()(const std::string& __x) const
	{
		return __stl_hash_string(__x.c_str());
	}
};

template<> struct hash<const void*>
{
	// Nemesi hash function (see: http://mail-index.netbsd.org/tech-kern/2001/11/30/0001.html)
	// treating the pointer as an array of bytes that is hashed.
	size_t operator()(const void* __x) const
	{
		union {
			const void* ptr;
			u8 bytes[sizeof(void*)];
		} val;
		size_t h = 5381;

		val.ptr = __x;

		for(size_t i = 0; i < sizeof(val); ++i)
			h = 257*h + val.bytes[i];

		return 257*h;
	}
};

}
#endif // GCC_VERSION


#elif STL_DINKUMWARE

# include <hash_map>
# include <hash_set>
// VC7 or above
# if MSC_VERSION >= 1300
#  define STL_HASH_MAP stdext::hash_map
#  define STL_HASH_MULTIMAP stdext::hash_multimap
#  define STL_HASH_SET stdext::hash_set
#  define STL_HASH_MULTISET stdext::hash_multiset
#  define STL_HASH_VALUE stdext::hash_value
// VC6 and anything else (most likely name)
# else
#  define STL_HASH_MAP std::hash_map
#  define STL_HASH_MULTIMAP std::hash_multimap
#  define STL_HASH_SET std::hash_set
#  define STL_HASH_MULTISET std::hash_multiset
#  define STL_HASH_VALUE std::hash_value
# endif	// MSC_VERSION >= 1300

#endif


// nonstandard STL containers
#define HAVE_STL_SLIST 0
#if STL_DINKUMWARE
# define HAVE_STL_HASH 1
#else
# define HAVE_STL_HASH 0
#endif

#endif	// #ifndef INCLUDED_STL
