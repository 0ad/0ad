#ifndef SYSDEP_H__
#define SYSDEP_H__

#include "config.h"

#include "sysdep/debug.h"

#ifdef _WIN32
#include "win/win.h"
#include "win/wdbg.h"
#elif defined(OS_UNIX)
#include "unix/unix.h"
#endif

#ifdef _WIN32
#include "lib/sysdep/win/printf.h"
#else
#define vsnprintf2 vsnprintf
#endif

#ifdef __cplusplus
extern "C" {
#endif


extern void display_msg(const char* caption, const char* msg);
extern void wdisplay_msg(const wchar_t* caption, const wchar_t* msg);


extern int clipboard_set(const wchar_t* text);
extern wchar_t* clipboard_get(void);
extern int clipboard_free(wchar_t* copy);


extern int get_executable_name(char* n_path, size_t buf_size);

extern int pick_directory(char* n_path, size_t buf_size);


#ifdef _MSC_VER
extern double round(double);
#endif

#ifndef HAVE_C99
extern float fminf(float a, float b);
extern float fmaxf(float a, float b);
#endif

#ifndef _MSC_VER
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


#ifdef __cplusplus
}
#endif


// C++ linkage

// STL_HASH_MAP, STL_HASH_MULTIMAP
#ifdef __GNUC__
// GCC
# include <ext/hash_map>
# define STL_HASH_MAP __gnu_cxx::hash_map
# define STL_HASH_MULTIMAP __gnu_cxx::hash_multimap

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
}

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
