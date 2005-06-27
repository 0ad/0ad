#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED


//-----------------------------------------------------------------------------
// user-specified configuration choices
//-----------------------------------------------------------------------------

#undef CONFIG_DISABLE_EXCEPTIONS

#undef CONFIG_USE_MMGR


//-----------------------------------------------------------------------------
// auto-detect OS and platform via predefined macros
//-----------------------------------------------------------------------------

// get compiler versions with consistent names + format:
// (major*100 + minor), or 0 if not present. note that more than
// one *_VERSION may be non-zero due to interoperability (e.g. ICC with MSC).
// .. ICC
#if defined(__INTEL_COMPILER)
# define ICC_VERSION __INTEL_COMPILER
#else
# define ICC_VERSION 0
#endif
// .. VC
#ifdef _MSC_VER
# define MSC_VERSION _MSC_VER
#else
# define MSC_VERSION 0
#endif
// .. GCC
#ifdef __GNUC__
# define GCC_VERSION (__GNUC__*100 + __GNUC_MINOR__)
#else
# define GCC_VERSION 0
#endif

// STL
#if defined(_CPPLIB_VER)
# define STL_DINKUMWARE _CPPLIB_VER
#else
# define STL_DINKUMWARE 0
#endif

// OS
// .. Windows
#if defined(_WIN32) || defined(WIN32)
# define OS_WIN
// .. Linux
#elif defined(linux) || defined(__linux) || defined(__linux__)
# define OS_LINUX
# define OS_UNIX
// .. Mac OS X
#elif defined(MAC_OS_X
# define OS_MACOSX
# define OS_UNIX
// .. BSD
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define OS_BSD
# define OS_UNIX
// .. Solaris
#elif defined(SOLARIS)
# define OS_SOLARIS
# define OS_UNIX
// .. BeOS
#elif defined(__BEOS__)
# define OS_BEOS
// .. Mac OS 9 or below
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
# define OS_MAC
// .. Amiga
#elif defined(__amigaos__)
# define OS_AMIGA
// .. Unix-based
#elif defined(unix) || defined(__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
# define OS_UNIX
// .. unknown
#else
# error "unknown OS - add define here"
#endif

// byte order
#if defined(__i386__) || defined(__i386) || defined(_M_IX86) || \
    defined(__ia64__) || defined(__ia64) || defined(_M_IA64) || \
	defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA) || \
	defined(__arm__) || \
	defined(__MIPSEL__) || \
	defined(__LITTLE_ENDIAN__)
# define LITTLE_ENDIAN
#else
# define BIG_ENDIAN
#endif


//-----------------------------------------------------------------------------
// auto-detect platform features, given the above information
//-----------------------------------------------------------------------------

// compiler support for C99
// (this is more convenient than testing __STDC_VERSION__ directly)
#undef HAVE_C99
#ifdef __STDC_VERSION__		// nested #if to avoid ICC warning if not defined
# if __STDC_VERSION__ >= 199901L
#  define HAVE_C99
# endif
#endif

// gettimeofday()
#undef HAVE_GETTIMEOFDAY
#ifdef OS_UNIX
# define HAVE_GETTIMEOFDAY
#endif

// X server
#undef HAVE_X
#ifdef OS_LINUX
# define HAVE_X
#endif

// __asm{} blocks (Intel syntax)
#undef HAVE_ASM
#if (MSC_VERSION != 0)
# define HAVE_ASM
#endif

// precompiled headers (affects what precompiled.h pulls in; see there)
#undef HAVE_PCH
#if (MSC_VERSION != 0) || (GCC_VERSION > 304)
# define HAVE_PCH
#endif

// VC debug memory allocator / leak detector
#undef HAVE_VC_DEBUG_ALLOC
#if (MSC_VERSION != 0)
# define HAVE_VC_DEBUG_ALLOC
#endif
// .. only in full-debug mode;
#if	defined(NDEBUG) || defined(TESTING)
# undef HAVE_VC_DEBUG_ALLOC
#endif
// .. require PCH, because it makes sure system headers are included before
//    redefining new (otherwise, tons of errors result);
#if !defined(HAVE_PCH)
# undef HAVE_VC_DEBUG_ALLOC
#endif
// .. disable on ICC9, because the ICC 9.0.006 beta appears to generate
//    incorrect code when we redefine new.
//    TODO: remove when no longer necessary
#if ICC_VERSION == 900
# undef HAVE_VC_DEBUG_ALLOC
#endif

// _CPPLIB_VER

#endif	// #ifndef CONFIG_H_INCLUDED
