/**
 * =========================================================================
 * File        : sysdep.h
 * Project     : 0 A.D.
 * Description : various system-specific function implementations
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef SYSDEP_H_INCLUDED
#define SYSDEP_H_INCLUDED

#include "lib/config.h"
#include "lib/debug.h"	// ErrorReaction

#include <cmath>	// see comments below about isfinite
#include <cstdarg>	// needed for vsnprintf2

// some functions among the sysdep API are implemented as macros
// that redirect to the platform-dependent version. this is done where
// the cost of a trampoline function would be too great; VC7 does not
// always inline them.
// we therefore need to include those headers.
#if OS_WIN
# include "lib/sysdep/win/win.h"
#elif OS_UNIX
# include "lib/sysdep/unix/unix.h"
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


// try to define _W64, if not already done
// (this is useful for catching pointer size bugs)
#ifndef _W64
# if MSC_VERSION
#  define _W64 __w64
# elif GCC_VERSION
#  define _W64 __attribute__((mode (__pointer__)))
# else
#  define _W64
# endif
#endif


//-----------------------------------------------------------------------------
// C99 / SUSv3 emulation where needed
//-----------------------------------------------------------------------------

// vsnprintf2: doesn't quite follow the standard for vsnprintf, but works
// better across compilers:
// - handles positional parameters and %lld
// - always null-terminates the buffer
// - returns -1 on overflow (if the output string (including null) does not fit in the buffer)
extern int vsnprintf2(char* buffer, size_t count, const char* format, va_list argptr);

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

// emulate some C99 functions if not already available:
//   rint: round float to nearest integral value, according to
//     current rounding mode.
//   fminf/fmaxf: return minimum/maximum of two floats.
#if !HAVE_C99_MATH
// .. fast IA-32 versions
# if CPU_IA32
#  define rintf ia32_rintf
#  define rint ia32_rint
#  define fminf ia32_fminf
#  define fmaxf ia32_fmaxf

#  define FP_NAN       IA32_FP_NAN
#  define FP_NORMAL    IA32_FP_NORMAL
#  define FP_INFINITE  (FP_NAN | FP_NORMAL)
#  define FP_ZERO      IA32_FP_ZERO
#  define FP_SUBNORMAL (FP_NORMAL | FP_ZERO)
#  define fpclassify(x) ( (sizeof(x) == sizeof(float))? ia32_fpclassifyf(x) : ia32_fpclassify(x) )
// .. portable C emulation
# else
   extern float rintf(float f);
   extern double rint(double d);
   extern float fminf(float a, float b);
   extern float fmaxf(float a, float b);

#  define FP_NAN       1
#  define FP_NORMAL    2
#  define FP_INFINITE  (FP_NAN | FP_NORMAL)
#  define FP_ZERO      4
#  define FP_SUBNORMAL (FP_NORMAL | FP_ZERO)
   extern uint fpclassify(double d);
# endif

# define isnan(d) (fpclassify(d) == FP_NAN)
# define isfinite(d) ((fpclassify(d) & (FP_NORMAL|FP_ZERO)) != 0)
# define isinf(d) (fpclassify(d) == FP_NAN|FP_NORMAL)
# define isnormal(d) (fpclassify(d) == FP_NORMAL)
//# define signbit
#else	// HAVE_C99_MATH
// Some systems have C99 support but in C++ they provide only std::isfinite
// and not isfinite. C99 specifies that isfinite is a macro, so we can use
// #ifndef and define it if it's not there already.
// We've included <cmath> above to make sure it defines that macro.
# ifndef isfinite
#  define isfinite std::isfinite
#  define isnan std::isnan
#  define isinf std::isinf
#  define isnormal std::isnormal
# endif
#endif	// HAVE_C99_MATH

// C99-like restrict (non-standard in C++, but widely supported in various forms).
//
// May be used on pointers. May also be used on member functions to indicate
// that 'this' is unaliased (e.g. "void C::m() RESTRICT { ... }").
// Must not be used on references - GCC supports that but VC doesn't.
//
// We call this "RESTRICT" to avoid conflicts with VC's __declspec(restrict),
// and because it's not really the same as C99's restrict.
//
// To be safe and satisfy the compilers' stated requirements: an object accessed
// by a restricted pointer must not be accessed by any other pointer within the
// lifetime of the restricted pointer, if the object is modified.
// To maximise the chance of optimisation, any pointers that could potentially
// alias with the restricted one should be marked as restricted too.
//
// It would probably be a good idea to write test cases for any code that uses
// this in an even very slightly unclear way, in case it causes obscure problems
// in a rare compiler due to differing semantics.
//
// .. we made g++ claim to support C99 by defining __STDC_VERSION__ in the
//    makefiles, but that's wrong and it doesn't have the restrict keyword.
//    use the extension __restrict__ instead.
#if GCC_VERSION
# define RESTRICT __restrict__
// .. already available in C99 as 'restrict'
#elif HAVE_C99
# define RESTRICT restrict
// .. VC8 provides __restrict
#elif MSC_VERSION >= 1400
# define RESTRICT __restrict
// .. ICC supports the keyword 'restrict' when run with the /Qrestrict option,
//    but it always also supports __restrict__ or __restrict to be compatible
//    with GCC/MSVC, so we'll use the underscored version. One of {GCC,MSC}_VERSION
//    should have been defined in addition to ICC_VERSION, so we should be using
//    one of the above cases (unless it's an old VS7.1-emulating ICC).
#elif ICC_VERSION
# error ICC_VERSION defined without either GCC_VERSION or an adequate MSC_VERSION
// .. unsupported; remove it from code
#else
# define RESTRICT
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

#if !HAVE_STRDUP
extern char* strdup(const char* str);
extern wchar_t* wcsdup(const wchar_t* str);
#endif	// #if !HAVE_STRDUP


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

// show the error dialog. flags: see DebugDisplayErrorFlags.
// called from debug_display_error.
// can be overridden by means of ah_display_error.
extern ErrorReaction sys_display_error(const wchar_t* text, uint flags);


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
// cursor is only valid when INFO::OK is returned; in that case, it must be
//   sys_cursor_free-ed when no longer needed.
extern LibError sys_cursor_create(uint w, uint h, void* bgra_img,
	uint hx, uint hy, void** cursor);

// create a fully transparent cursor (i.e. one that when passed to set hides
// the system cursor)
extern LibError sys_cursor_create_empty(void **cursor);

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
extern LibError sys_cursor_set(void* cursor);

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
extern LibError sys_cursor_free(void* cursor);


//
// misc
//

// describe the current OS error state.
//
// err: if not 0, use that as the error code to translate; otherwise,
// uses GetLastError or similar.
// rationale: it is expected to be rare that OS return/error codes are
// actually seen by user code, but we leave the possibility open.
extern LibError sys_error_description_r(int err, char* buf, size_t max_chars);

// determine filename of the module to whom the given address belongs.
// useful for handling exceptions in other modules.
// <path> receives full path to module; it must hold at least MAX_PATH chars.
// on error, it is set to L"".
// return path for convenience.
wchar_t* sys_get_module_filename(void* addr, wchar_t* path);

// store full path to the current executable.
// useful for determining installation directory, e.g. for VFS.
extern LibError sys_get_executable_name(char* n_path, size_t buf_size);

// have the user specify a directory via OS dialog.
// stores its full path in the given buffer, which must hold at least
// PATH_MAX chars.
extern LibError sys_pick_directory(char* n_path, size_t buf_size);

// execute the specified function once on each CPU.
// this includes logical HT units and proceeds serially (function
// is never re-entered) in order of increasing OS CPU ID.
// note: implemented by switching thread affinity masks and forcing
// a reschedule, which is apparently not possible with POSIX.
//
// may fail if e.g. OS is preventing us from running on some CPUs.
// called from ia32.cpp get_cpu_count.
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


// return the largest sector size [bytes] of any storage medium
// (HD, optical, etc.) in the system.
//
// this may be a bit slow to determine (iterates over all drives),
// but caches the result so subsequent calls are free.
// (caveat: device changes won't be noticed during this program run)
//
// sector size is relevant because Windows aio requires all IO
// buffers, offsets and lengths to be a multiple of it. this requirement
// is also carried over into the vfs / file.cpp interfaces for efficiency
// (avoids the need for copying to/from align buffers).
//
// waio uses the sector size to (in some cases) align IOs if
// they aren't already, but it's also needed by user code when
// aligning their buffers to meet the requirements.
//
// the largest size is used so that we can read from any drive. while this
// is a bit wasteful (more padding) and requires iterating over all drives,
// it is the only safe way: this may be called before we know which
// drives will be needed, and hardlinks may confuse things.
extern size_t sys_max_sector_size();

#if OS_WIN
# define SYS_DIR_SEP '\\'
#else
# define SYS_DIR_SEP '/'
#endif

// tell the compiler that the code at/following this macro invocation is
// unreachable. this can improve optimization and avoid warnings.
//
// this macro should not generate any fallback code; it is merely the
// compiler-specific backend for lib.h's UNREACHABLE.
// #define it to nothing if the compiler doesn't support such a hint.
#if MSC_VERSION
# define SYS_UNREACHABLE __assume(0)
#else
# define SYS_UNREACHABLE
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

#endif	// #ifndef SYSDEP_H_INCLUDED
