//
// OS
//

// Windows
#if defined(_WIN32) || defined(WIN32)
# define OS_WIN
// Linux
#elif defined(linux) || defined(__linux) || defined(__linux__)
# define OS_LINUX
# define OS_UNIX
// Mac OS X
#elif defined(MAC_OS_X
# define OS_MACOSX
# define OS_UNIX
// Mac OS 9 or below
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
# define OS_MACOS
// BSD
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# define OS_BSD
// Solaris
#elif defined(SOLARIS)
# define OS_SOLARIS
// BeOS
#elif defined(__BEOS__)
# define OS_BEOS
// Amiga
#elif defined(__amigaos__)
# define OS_AMIGA
// Unix-based
#elif defined(unix) || defined(__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
# define OS_UNIX
#else
# error "unknown OS - add define here"
#endif


#undef HAVE_C99		// compiler advertises support for C99

#undef HAVE_ASM

#undef HAVE_GETTIMEOFDAY
#undef HAVE_X

#undef HAVE_PCH

#undef HAVE_DEBUGALLOC

#undef CONFIG_DISABLE_EXCEPTIONS


#ifdef __STDC_VERSION__		// nested #if to avoid ICC warning if not defined
# if __STDC_VERSION__ >= 199901L
#  define HAVE_C99
# endif
#endif

#ifdef _MSC_VER
# define HAVE_ASM
# define HAVE_PCH
#endif

#if defined(_MSC_VER) && defined(HAVE_PCH) && !( defined(NDEBUG) || defined(TESTING) )
# define HAVE_DEBUGALLOC
#endif

#ifdef OS_UNIX
# define HAVE_GETTIMEOFDAY
#endif

#ifdef OS_LINUX
# define HAVE_X
#endif


