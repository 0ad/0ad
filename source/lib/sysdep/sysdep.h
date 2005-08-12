#ifndef SYSDEP_H_INCLUDED
#define SYSDEP_H_INCLUDED

#include "config.h"

#if OS_WIN
# include "win/win.h"
#elif OS_UNIX
# include "unix/unix.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// compiling without exceptions (usually for performance reasons);
// tell STL not to generate any.
#if CONFIG_DISABLE_EXCEPTIONS
# if OS_WIN
#  define _HAS_EXCEPTIONS 0
# else
#  define STL_NO_EXCEPTIONS
# endif
#endif


// vsnprintf2: handles positional parameters and %lld.
// already available on *nix, emulated on Win32.
#if OS_WIN
extern int vsnprintf2(char* buffer, size_t count, const char* format, va_list argptr);
#else
#define vsnprintf2 vsnprintf
#endif

// alloca: allocate on stack, automatically free, return 0 if out of mem.
// already available on *nix, emulated on Win32.
#if OS_WIN
#undef alloca	// from malloc.h
extern void* alloca(size_t size);
#endif

// finite: return 0 iff the given double is infinite or NaN.
#if OS_WIN
# define finite _finite
#else
# define finite __finite
#endif


//
// output
//

enum DisplayErrorFlags
{
	DE_ALLOW_SUPPRESS = 1,
	DE_NO_CONTINUE = 2,
	DE_MANUAL_BREAK = 4
};

// choices offered by the shared error dialog
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

extern ErrorReaction display_error(const wchar_t* description, int flags,
	uint skip, void* context, const char* file, int line);

// convenience version, in case the advanced parameters aren't needed.
// done this way instead of with default values so that it also works in C.
#define DISPLAY_ERROR(text) display_error(text, 0, 0, 0, __FILE__, __LINE__)

// internal use only (used by display_error)
extern ErrorReaction display_error_impl(const wchar_t* text, int flags);



extern void display_msg(const char* caption, const char* msg);
extern void wdisplay_msg(const wchar_t* caption, const wchar_t* msg);


//
// clipboard
//

extern int clipboard_set(const wchar_t* text);
extern wchar_t* clipboard_get(void);
extern int clipboard_free(wchar_t* copy);


//
// mouse cursor
//

// note: these do not warn on error; that is left to the caller.

// creates a cursor from the given 32 bpp RGBA texture. hotspot (hx,hy) is
// the offset from its upper-left corner to the position where mouse clicks
// are registered.
// the cursor must be cursor_free-ed when no longer needed.
extern int sys_cursor_create(int w, int h, void* img, int hx, int hy,
	void** cursor);

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
extern int sys_cursor_set(void* cursor);

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
extern int sys_cursor_free(void* cursor);


extern int get_executable_name(char* n_path, size_t buf_size);

// return filename of the module which contains address <addr>,
// or L"" on failure. path holds the string and must be >= MAX_PATH chars.
wchar_t* get_module_filename(void* addr, wchar_t* path);


extern int pick_directory(char* n_path, size_t buf_size);


#if MSC_VERSION
extern double round(double);
#endif

#if !HAVE_C99
extern float fminf(float a, float b);
extern float fmaxf(float a, float b);
#endif

#if !MSC_VERSION
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif


#ifdef __cplusplus
}
#endif


// C++ linkage

// STL_HASH_MAP, STL_HASH_MULTIMAP, STL_HASH_SET
#if GCC_VERSION
// GCC
# include <ext/hash_map>
# include <ext/hash_set> // Probably?

# define STL_HASH_MAP __gnu_cxx::hash_map
# define STL_HASH_MULTIMAP __gnu_cxx::hash_multimap
# define STL_HASH_SET __gnu_cxx::hash_set
# define STL_HASH_MULTISET __gnu_cxx::hash_multiset
# define STL_SLIST __gnu_cxx::slist

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
// VC7 or above
# if MSC_VERSION >= 1300
#  define STL_HASH_MAP stdext::hash_map
#  define STL_HASH_MULTIMAP stdext::hash_multimap
#  define STL_HASH_SET stdext::hash_set
#  define STL_HASH_MULTISET stdext::hash_multiset
// VC6 and anything else (most likely name)
# else
#  define STL_HASH_MAP std::hash_map
#  define STL_HASH_MULTIMAP std::hash_multimap
#  define STL_HASH_SET std::hash_set
#  define STL_HASH_MULTISET std::hash_multiset
# endif	// MSC_VERSION >= 1300
#endif	// !__GNUC__

#include "debug.h"

#endif	// #ifndef SYSDEP_H_INCLUDED
