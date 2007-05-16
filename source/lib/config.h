/**
 * =========================================================================
 * File        : config.h
 * Project     : 0 A.D.
 * Description : compile-time configuration and capability detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_CONFIG
#define INCLUDED_CONFIG

// the config/have macros are always defined; their values (1 or 0) are
// tested with #if instead of #ifdef.
// this protects user code from typos such as #ifdef _MSC_VEER, which
// would silently remove code. instead, we will at least get "test of
// undefined macro" warnings here. not including this header also triggers
// such warnings, but won't happen often because we're included from PCH.


//-----------------------------------------------------------------------------
// user-specified configuration choices
//-----------------------------------------------------------------------------

// allow override via compiler settings by checking #ifndef.

// omit frame pointers - stack frames will not be generated, which
// improves performance but makes walking the stack harder.
// this is acted upon by #pragmas in sysdep.h.
#ifndef CONFIG_OMIT_FP
# ifdef NDEBUG
#  define CONFIG_OMIT_FP 1
# else
#  define CONFIG_OMIT_FP 0
# endif
#endif

// (only applicable if CPU_IA32) 64-bit values will be returned in EDX:EAX.
// this chiefly affects ia32_rdtsc. if not set, a safer but slower fallback
// will be used that doesn't assume anything about return convention.
#ifndef CONFIG_RETURN64_EDX_EAX
# define CONFIG_RETURN64_EDX_EAX 1
#endif

// allow use of RDTSC for raw tick counts (otherwise, the slower but
// more reliable on MP systems wall-clock will be used).
#ifndef CONFIG_TIMER_ALLOW_RDTSC
# if CPU_IA32
#  define CONFIG_TIMER_ALLOW_RDTSC 1
# else
#  define CONFIG_TIMER_ALLOW_RDTSC 0
# endif
#endif

// this enables/disables the actual checking done by OverrunProtector-s
// (quite slow, entailing mprotect() before/after each access).
// define to 1 here or in the relevant module if you suspect mem corruption.
// we provide this option because OverrunProtector requires some changes to
// the object being wrapped, and we want to leave those intact but not
// significantly slow things down except when needed.
#ifndef CONFIG_OVERRUN_PROTECTION
# define CONFIG_OVERRUN_PROTECTION 0
#endif

// enable memory tracking (slow). see mmgr.cpp.
#ifndef CONFIG_USE_MMGR
# define CONFIG_USE_MMGR 0
#endif

// enable additional debug checks (very slow).
#ifndef CONFIG_PARANOIA
# define CONFIG_PARANOIA 0
#endif

// finale release; disables some safety checks.
#ifndef CONFIG_FINAL
# define CONFIG_FINAL 0
#endif


// enable trace output for low-level code - various functions will
// debug_printf when they are entered/exited. note that the appropriate
// TRACEn tags must be debug_filter_add-ed for this to have any effect.
#ifndef CONFIG_TRACE
# define CONFIG_TRACE 0
#endif

// try to prevent any exceptions from being thrown - even by the C++
// standard library. useful only for performance tests.
#ifndef CONFIG_DISABLE_EXCEPTIONS
# define CONFIG_DISABLE_EXCEPTIONS 0
#endif


//-----------------------------------------------------------------------------
// auto-detect OS and platform via predefined macros
//-----------------------------------------------------------------------------

// rationale:
// - these macros have consistent names and numerical values; using
//   them saves other code from having to know the obscure predefined macros.
// - we'd like to use #if/#elif/#endif chains for e.g. OS_* to allow warning
//   if none is selected, but there's no good way to #define all inapplicable
//   settings to 0. doing so up front is hard to maintain and would require
//   #undef before setting any one to 1. #ifndef afterwards for each setting
//   is ugly and brittle as well. we therefore use #if/#else/#endif.

// compiler
// 0 if not present, or (major*100 + minor). note that more than
// one *_VERSION may be non-zero due to interoperability (e.g. ICC with MSC).
// .. VC
#ifdef _MSC_VER
# define MSC_VERSION _MSC_VER
#else
# define MSC_VERSION 0
#endif
// .. ICC (VC-compatible)
#if defined(__INTEL_COMPILER)
# define ICC_VERSION __INTEL_COMPILER
#else
# define ICC_VERSION 0
#endif
// .. LCC (VC-compatible)
#if defined(__LCC__)
# define LCC_VERSION __LCC__
#else
# define LCC_VERSION 0
#endif
// .. GCC
#ifdef __GNUC__
# define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__)
#else
# define GCC_VERSION 0
#endif


// STL
// (checked by STL implementation-specific code in debug_stl).
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


// OS
// .. Windows
#if defined(_WIN32)
# define OS_WIN 1
#else
# define OS_WIN 0
#endif
// .. Linux
#if defined(linux) || defined(__linux) || defined(__linux__)
# define OS_LINUX 1
#else
# define OS_LINUX 0
#endif
// .. Mac OS X
#if (defined(__APPLE__) && defined(__MACH__))
# define OS_MACOSX 1
#else
# define OS_MACOSX 0
#endif
// .. BSD
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define OS_BSD 1
#else
# define OS_BSD 0
#endif
// .. Solaris
#if defined(sun) || defined(__sun)
# define OS_SOLARIS 1
#else
# define OS_SOLARIS 0
#endif
// .. BeOS
#if defined(__BEOS__)
# define OS_BEOS 1
#else
# define OS_BEOS 0
#endif
// .. Mac OS 9 or below
#if defined(macintosh)
# define OS_MAC 1
#else
# define OS_MAC 0
#endif
// .. Amiga
#if defined(AMIGA)
# define OS_AMIGA 1
#else
# define OS_AMIGA 0
#endif
// .. Unix-based
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
# define OS_UNIX 1
#else
# define OS_UNIX 0
#endif

// convenience: additionally set OS_UNIX for Unix-based OSes
// note: doing this in an separate section instead of adding the extra define
// to all affected OSes saves a few undefs or macro redefinition warnings.
#if OS_LINUX || OS_MACOSX || OS_BSD || OS_SOLARIS
# undef OS_UNIX
# define OS_UNIX 1
#endif

// convenience: additionally set OS_BSD for BSD-based OSes. see note above.
#if OS_MACOSX
# undef OS_BSD
# define OS_BSD 1
#endif


// CPU
// .. IA-32
#if defined(_M_IX86) || defined(i386) || defined(_X86_)
# define CPU_IA32 1
#else
# define CPU_IA32 0
#endif
// .. IA-64
#if defined(_M_IA64) || defined(__ia64__)
# define CPU_IA64 1
#else
# define CPU_IA64 0
#endif
// .. AMD64
#if defined(_M_AMD64) || defined(__amd64__) || defined(__amd64)
# define CPU_AMD64 1
#else
# define CPU_AMD64 0
#endif
// .. Alpha
#if defined(_M_ALPHA) || defined(__alpha__) || defined(__alpha)
# define CPU_ALPHA 1
#else
# define CPU_ALPHA 0
#endif
// .. ARM
#if defined(__arm__)
# define CPU_ARM 1
#else
# define CPU_ARM 0
#endif
// .. MIPS
#if defined(__MIPS__) || defined(__mips__) || defined(__mips)
# define CPU_MIPS 1
#else
# define CPU_MIPS 0
#endif


// byte order
// if already defined by someone else, assume they are also correct :P
#ifndef BYTE_ORDER
# define LITTLE_ENDIAN 0x4321
# define BIG_ENDIAN    0x1234
# if CPU_IA32 || CPU_IA64 || CPU_AMD64 || CPU_ALPHA || CPU_ARM || CPU_MIPS || defined(__LITTLE_ENDIAN__)
#  define BYTE_ORDER LITTLE_ENDIAN
# else
#  define BYTE_ORDER BIG_ENDIAN
# endif
#endif


//-----------------------------------------------------------------------------
// auto-detect platform features, given the above information
//-----------------------------------------------------------------------------

// check if compiling in pure C mode (not C++) with support for C99.
// (this is more convenient than testing __STDC_VERSION__ directly)
//
// note: C99 provides several useful but disjunct bits of functionality.
// unfortunately, most C++ compilers do not offer a complete implementation.
// however, many of these features are likely to be added to C++, and/or are
// already available as extensions. what we'll do is add a HAVE_ macro for
// each feature and test those instead. they are set if HAVE_C99, or also if
// the compiler happens to support something compatible.
//
// rationale: lying about __STDC_VERSION__ via Premake so as to enable support
// for some C99 functions doesn't work. Mac OS X headers would then use the
// restrict keyword, which is never supported by g++ (because that might
// end up breaking valid C++98 programs).
#define HAVE_C99 0
#ifdef __STDC_VERSION__
# if __STDC_VERSION__ >= 199901L
#  undef  HAVE_C99
#  define HAVE_C99 1
# endif
#endif

// strdup, wcsdup
#if OS_MACOSX
# define HAVE_STRDUP 1
# define HAVE_WCSDUP 0
#else
# define HAVE_STRDUP 1
# define HAVE_WCSDUP 1
#endif

// emulation needed on VC8 because this function is "deprecated"
#if MSC_VERSION >= 1400
# define HAVE_MKDIR 0
#else
# define HAVE_MKDIR 1
#endif

// rint*, fminf, fpclassify (too few/diverse to make separate HAVE_ for each)
#if HAVE_C99 || GCC_VERSION
# define HAVE_C99_MATH 1
#else
# define HAVE_C99_MATH 0
#endif

// gettimeofday()
#if OS_UNIX
# define HAVE_GETTIMEOFDAY 1
#else
# define HAVE_GETTIMEOFDAY 0
#endif

// clock_gettime()
#define HAVE_CLOCK_GETTIME 0
#if OS_UNIX
# include <unistd.h>
# if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
#  undef HAVE_CLOCK_GETTIME
#  define HAVE_CLOCK_GETTIME 1
# endif
#endif

// X server
#if OS_LINUX
# define HAVE_X 1
#else
# define HAVE_X 0
#endif

// MSVC/ICC-style __asm{} blocks (Intel syntax)
#if MSC_VERSION
# define HAVE_MS_ASM 1
#else
# define HAVE_MS_ASM 0
#endif

// GNU-style __asm() blocks (AT&T syntax)
#if GCC_VERSION
# define HAVE_GNU_ASM 1
#else
# define HAVE_GNU_ASM 0
#endif

// precompiled headers (affects what precompiled.h pulls in; see there)
#if MSC_VERSION
# define HAVE_PCH 1
#elif defined(USING_PCH)
# define HAVE_PCH 1
#else
# define HAVE_PCH 0
#endif

// VC debug memory allocator / leak detector
// notes:
// - PCH is required because it makes sure system headers are included
//   before redefining new (otherwise, tons of errors result);
// - disabled on ICC9 because the ICC 9.0.006 beta appears to generate
//   incorrect code when we redefine new.
//   TODO: remove when no longer necessary
#if MSC_VERSION && \
	(!defined(NDEBUG) || defined(TESTING)) && \
	HAVE_PCH && \
	ICC_VERSION != 900
# define HAVE_VC_DEBUG_ALLOC 1
#else
# define HAVE_VC_DEBUG_ALLOC 0
#endif

// nonstandard STL containers
#define HAVE_STL_SLIST 0
#if STL_DINKUMWARE
# define HAVE_STL_HASH 1
#else
# define HAVE_STL_HASH 0
#endif

// safe CRT functions: strcpy_s, fopen_s, etc.
// these are always available to users: if not provided by the CRT, we
// implement them ourselves. this flag is only used to skip our impl.
#if MSC_VERSION >= 1400 || GCC_VERSION
# define HAVE_SECURE_CRT 1
#else
# define HAVE_SECURE_CRT 0
#endif


// should we use our float->int code? it requires IA32 and inline asm;
// new GCC with -ffast-math and VC8 can use SSE, so skip it there.
#if CPU_IA32 && HAVE_MS_ASM && MSC_VERSION && MSC_VERSION < 1400
# define USE_IA32_FLOAT_TO_INT 1
#else
# define USE_IA32_FLOAT_TO_INT 0
#endif

#endif	// #ifndef INCLUDED_CONFIG
