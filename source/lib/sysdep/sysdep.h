#ifndef SYSDEP_H_INCLUDED
#define SYSDEP_H_INCLUDED

#include "config.h"

#include "sysdep/debug.h"

#ifdef _WIN32
#include "win/win.h"
#include "win/wdbg.h"
#elif defined(OS_UNIX)
#include "unix/unix.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


// vsnprintf2: handles positional parameters and %lld.
// already available on *nix, emulated on Win32.
#ifdef _WIN32
extern int vsnprintf2(char* buffer, size_t count, const char* format, va_list argptr);
#else
#define vsnprintf2 vsnprintf
#endif




enum DisplayErrorFlags
{
	DE_ALLOW_SUPPRESS = 1,
	DE_NO_CONTINUE = 2,
	DE_MANUAL_BREAK = 4
};

// user choices in the assert/unhandled exception dialog.
enum ErrorReaction
{
	// ignore, continue as if nothing happened.
	ER_CONTINUE = 1,
		// note: don't start at 0 because that is interpreted as a
		// DialogBoxParam failure.

	// ignore and do not report again.
	// only returned if DE_ALLOW_SUPPRESS was passed.
	ER_SUPPRESS,
		// note: non-persistent; only applicable during this program run.

	// trigger breakpoint, i.e. enter debugger.
	// only returned if DE_MANUAL_BREAK was passed; otherwise,
	// display_error will trigger a breakpoint itself.
	ER_BREAK,

	// exit the program immediately.
	// never returned; display_error exits immediately.
	ER_EXIT
};


extern ErrorReaction display_error_impl(const wchar_t* text, int flags);

#define DISPLAY_ERROR(text) display_error(text, 0, 0, 0, __FILE__, __LINE__)

extern ErrorReaction display_error(const wchar_t* text, int flags,
	uint skip, void* context, const char* file, int line);

extern void display_msg(const char* caption, const char* msg);
extern void wdisplay_msg(const wchar_t* caption, const wchar_t* msg);


extern int clipboard_set(const wchar_t* text);
extern wchar_t* clipboard_get(void);
extern int clipboard_free(wchar_t* copy);


extern int get_executable_name(char* n_path, size_t buf_size);

// return filename of the module which contains address <addr>,
// or L"" on failure. path holds the string and must be >= MAX_PATH chars.
wchar_t* get_module_filename(void* addr, wchar_t* path);


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

// STL_HASH_MAP, STL_HASH_MULTIMAP, STL_HASH_SET
#ifdef __GNUC__
// GCC
# include <ext/hash_map>
# include <ext/hash_set> // Probably?

# define STL_HASH_MAP __gnu_cxx::hash_map
# define STL_HASH_MULTIMAP __gnu_cxx::hash_multimap
# define STL_HASH_SET __gnu_cxx::hash_set

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
# include <hash_set>
# if defined(_MSC_VER) && (_MSC_VER >= 1300)
// VC7 or above
#  define STL_HASH_MAP stdext::hash_map
#  define STL_HASH_MULTIMAP stdext::hash_multimap
#  define STL_HASH_SET stdext::hash_set
# else
// VC6 and anything else (most likely name)
#  define STL_HASH_MAP std::hash_map
#  define STL_HASH_MULTIMAP std::hash_multimap
#  define STL_HASH_SET std::hash_set
# endif	// defined(_MSC_VER) && (_MSC_VER >= 1300)
#endif	// !__GNUC__


#endif	// #ifndef SYSDEP_H_INCLUDED
