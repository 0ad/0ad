/**
 * =========================================================================
 * File        : stl.h
 * Project     : 0 A.D.
 * Description : fixes for various STL implementations
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_STL
#define INCLUDED_STL

// compiling without exceptions (usually for performance reasons);
// tell STL not to generate any.
#if CONFIG_DISABLE_EXCEPTIONS
# if OS_WIN
#  define _HAS_EXCEPTIONS 0
# else
#  define STL_NO_EXCEPTIONS
# endif
#endif


// STL_HASH_MAP, STL_HASH_MULTIMAP, STL_HASH_SET
// these containers are useful but not part of C++98. most STL vendors
// provide them in some form; we hide their differences behind macros.

#if GCC_VERSION
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

// Hack: GCC Doesn't have a hash instance for std::string included (and it looks
// like they won't add it - marked resolved/wontfix in the gcc bugzilla)
namespace __gnu_cxx
{
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
				unsigned char bytes[sizeof(void*)];
			} val;
			size_t h = 5381;

			val.ptr = __x;

			for(uint i = 0; i < sizeof(val); ++i)
				h = 257*h + val.bytes[i];

			return 257*h;
		}
	};
}

#else	// !__GNUC__

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

#endif	// !__GNUC__

#endif	// #ifndef INCLUDED_STL
