#ifndef SYSDEP_H__
#define SYSDEP_H__

// STL_HASH_MAP, STL_HASH_MULTIMAP
#ifdef __GNUC__
// GCC
# include <ext/hash_map>
# define STL_HASH_MAP __gnu_cxx::hash_map
# define STL_HASH_MULTIMAP __gnu_cxx::hash_multimap
#else	// !__GNUC__
# include <hash_map>
# if defined(_MSC_VER) && (_MSC_VER >= 1300)
// VC7 or above
#  define STL_HASH_MAP stdext::hash_map
#  define STL_HASH_MULTIMAP stdext::hash_multimap
# else
// VC6 and anything else (most likely name)
#  define STL_HASH_MAP std::hash_map
#  define STL_HASH_MULTIMAP std::hash_multimap
# endif	// defined(_MSC_VER) && (_MSC_VER >= 1300)
#endif	// !__GNUC__


#endif	// #ifndef SYSDEP_H__
