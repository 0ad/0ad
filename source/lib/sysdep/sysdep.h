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

// pass "omit frame pointer" setting on to the compiler
#if MSC_VERSION
# if CONFIG_OMIT_FP
#  pragma optimize("y", on)
# else
#  pragma optimize("y", off)
# endif
#elif GCC_VERSION
// TODO
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

#ifdef CPU_IA32
# define memcpy2 ia32_memcpy
extern void* ia32_memcpy(void* dst, const void* src, size_t nbytes);
#else
# define memcpy2 memcpy
#endif

// rint: round float to nearest integer.
// provided by C99, otherwise:
#if !HAVE_C99
// .. implemented on IA-32; define as macro to avoid jmp overhead
# if CPU_IA32
#  define rintf ia32_rintf
#  define rint ia32_rint
# endif
// .. forward-declare either the IA-32 version or portable C emulation.
extern float rintf(float f);
extern double rint(double d);
#endif

// fast float->int conversion; does not specify rounding mode,
// so do not use them if exact values are needed.
#if USE_IA32_FLOAT_TO_INT
# define i32_from_float ia32_i32_from_float
# define i32_from_double ia32_i32_from_double
# define i64_from_double ia32_i64_from_double
#endif
// .. forward-declare either the IA-32 version or portable C emulation.
extern i32 i32_from_float(float);
extern i32 i32_from_double(double);
extern i64 i64_from_double(double);

// finite: return 0 iff the given double is infinite or NaN.
#if OS_WIN
# define finite _finite
#else
# define finite __finite
#endif

// C99 restrict
// .. for some reason, g++-3.3 claims to support C99 (according to
//    __STDC_VERSION__) but doesn't have the restrict keyword.
//    use the extension __restrict__ instead.
#if GCC_VERSION
# define restrict __restrict__
// .. already available; need do nothing
#elif HAVE_C99
// .. unsupported; remove it from code
#else
# define restrict
#endif

// C99 __func__
// .. already available; need do nothing
#if HAVE_C99
// .. VC supports something similar
#elif MSC_VERSION
# define __func__ __FUNCTION__
// .. unsupported; remove it from code
#else
# define __func__ "(unknown)"
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

// creates a cursor from the given texture file.
// hotspot (hx,hy) is the offset from its upper-left corner to the
// position where mouse clicks are registered.
// the cursor must be cursor_free-ed when no longer needed.
extern int sys_cursor_load(const char* filename,
	uint hx, uint hy, void** cursor);

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


// not possible with POSIX calls.
// called from ia32.cpp get_cpu_count
extern int on_each_cpu(void(*cb)());



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

#include "debug.h"

#endif	// #ifndef SYSDEP_H_INCLUDED
