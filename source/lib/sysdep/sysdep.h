#ifndef SYSDEP_H_INCLUDED
#define SYSDEP_H_INCLUDED

#include "config.h"

// some functions among the sysdep API are implemented as macros
// that redirect to the platform-dependent version. this is done where
// the cost of a trampoline function would be too great; VC7 does not
// always inline them.
// we therefore need to include those headers.
#if OS_WIN
# include "win/win.h"
#elif OS_UNIX
# include "unix/unix.h"
#endif
#if CPU_IA32
#include "ia32.h"
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


#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
// C99 / SUSv3 emulation where needed
//-----------------------------------------------------------------------------

// vsnprintf2: handles positional parameters and %lld.
// already available on *nix, emulated on Win32.
#if OS_WIN
extern int vsnprintf2(char* buffer, size_t count, const char* format, va_list argptr);
#else
#define vsnprintf2 vsnprintf
#endif

#if !HAVE_C99
extern float fminf(float a, float b);
extern float fmaxf(float a, float b);
#endif

#if !MSC_VERSION
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

// alloca: allocate on stack, automatically free, return 0 if out of mem.
// already available on *nix, emulated on Win32.
#if OS_WIN
#undef alloca	// from malloc.h
extern void* alloca(size_t size);
#endif

// rint: round float to nearest integral value.
// provided by C99, otherwise:
#if !HAVE_C99
// .. fast IA-32 version
# if CPU_IA32
#  define rintf ia32_rintf
#  define rint ia32_rint
// .. portable C emulation
# else
   extern float rintf(float f);
   extern double rint(double d);
# endif
#endif

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

#include "debug.h"

//-----------------------------------------------------------------------------
// sysdep API
//-----------------------------------------------------------------------------

//
// output
//

// raise a message box with the given text or (depending on platform)
// otherwise inform the user.
// called from debug_display_msgw.
extern void sys_display_msg(const char* caption, const char* msg);
extern void sys_display_msgw(const wchar_t* caption, const wchar_t* msg);

// show the error dialog. flags: see DisplayErrorFlags.
// called from debug_display_error.
extern ErrorReaction sys_display_error(const wchar_t* text, int flags);


//
// clipboard
//

// "copy" text into the clipboard. replaces previous contents.
extern LibError sys_clipboard_set(const wchar_t* text);

// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
extern wchar_t* sys_clipboard_get(void);

// frees memory used by <copy>, which must have been returned by
// sys_clipboard_get. see note above.
extern LibError sys_clipboard_free(wchar_t* copy);


//
// mouse cursor
//

// note: these do not warn on error; that is left to the caller.

// creates a cursor from the given image.
// w, h specify image dimensions [pixels]. limit is implementation-
//   dependent; 32x32 is typical and safe.
// bgra_img is the cursor image (BGRA format, bottom-up).
//   it is no longer needed and can be freed after this call returns.
// hotspot (hx,hy) is the offset from its upper-left corner to the
//   position where mouse clicks are registered.
// return: negative error code, or 0 on success. cursor is filled with
//   a pointer and undefined on failure. it must be sys_cursor_free-ed
//   when no longer needed.
extern LibError sys_cursor_create(uint w, uint h, void* bgra_img,
	uint hx, uint hy, void** cursor);

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
extern LibError sys_cursor_set(void* cursor);

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
extern LibError sys_cursor_free(void* cursor);


//
// misc
//

// OS-specific backend for error_description_r.
// NB: it is expected to be rare that OS return/error codes are actually
// seen by user code, but we still translate them for completeness.
extern LibError sys_error_description_r(int err, char* buf, size_t max_chars);

// determine filename of the module to whom the given address belongs.
// useful for handling exceptions in other modules.
// <path> receives full path to module; it must hold at least MAX_PATH chars.
// on error, it is set to L"".
// return path for convenience.
wchar_t* sys_get_module_filename(void* addr, wchar_t* path);

// store full path to the current executable.
// returns 0 or a negative error code.
// useful for determining installation directory, e.g. for VFS.
extern LibError sys_get_executable_name(char* n_path, size_t buf_size);

// have the user specify a directory via OS dialog.
// stores its full path in the given buffer, which must hold at least
// PATH_MAX chars.
// returns 0 on success or a negative error code.
extern LibError sys_pick_directory(char* n_path, size_t buf_size);

// execute the specified function once on each CPU.
// this includes logical HT units and proceeds serially (function
// is never re-entered) in order of increasing OS CPU ID.
// note: implemented by switching thread affinity masks and forcing
// a reschedule, which is apparently not possible with POSIX.
// return 0 on success or a negative error code on failure
// (e.g. if OS is preventing us from running on some CPUs).
// called from ia32.cpp get_cpu_count
extern LibError sys_on_each_cpu(void (*cb)());


// drop-in replacement for libc memcpy(). only requires CPU support for
// MMX (by now universal). highly optimized for Athlon and Pentium III
// microarchitectures; significantly outperforms VC7.1 memcpy and memcpy_amd.
// for details, see accompanying article.
#ifdef CPU_IA32
# define memcpy2 ia32_memcpy
extern void* ia32_memcpy(void* dst, const void* src, size_t nbytes);
#else
# define memcpy2 memcpy
#endif

// i32_from_float et al: convert float to int. much faster than _ftol2,
// which would normally be used by (int) casts.
// .. fast IA-32 version: only used in some cases; see macro definition.
#if USE_IA32_FLOAT_TO_INT
# define i32_from_float ia32_i32_from_float
# define i32_from_double ia32_i32_from_double
# define i64_from_double ia32_i64_from_double
// .. portable C emulation
#else
extern i32 i32_from_float(float);
extern i32 i32_from_double(double);
extern i64 i64_from_double(double);
#endif


#ifdef __cplusplus
}
#endif


//-----------------------------------------------------------------------------
// STL_HASH_MAP, STL_HASH_MULTIMAP, STL_HASH_SET
//-----------------------------------------------------------------------------

// these containers are useful but not part of C++98. most STL vendors
// provide them in some form; we hide their differences behind macros.

#if GCC_VERSION
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

	template<> struct hash<const void*>
	{
		// TODO: Is this a good hash function for pointers?
		// The basic idea is to avoid patterns that are caused by alignment of structures
		// in memory and of heap allocations
		size_t operator()(const void* __x) const
		{
			size_t val = (size_t)__x;
			const unsigned char* data = (unsigned char*)&val;
			size_t h = 0;

			for(uint i = 0; i < sizeof(val); ++i, ++data)
				h = 257*h + *data;

			return h;
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

#endif	// #ifndef SYSDEP_H_INCLUDED
