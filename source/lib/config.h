#if defined(_WIN32)
# define OS_WIN
#elif defined(linux)
# define OS_LINUX
# define OS_UNIX
#elif defined(macintosh)
# define OS_MACOS
#elif defined(__APPLE__)  && defined(__MACH__)
# define OS_MACOSX
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


