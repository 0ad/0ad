#ifndef INCLUDED_EXTERNAL_LIBRARIES_OPENMP
#define INCLUDED_EXTERNAL_LIBRARIES_OPENMP

// allows removing all OpenMP-related code via #define ENABLE_OPENMP 0
// before including this header. (useful during debugging, because the
// VC debugger isn't able to display OpenMP private variables)
#ifdef ENABLE_OPENMP
# if ENABLE_OPENMP && !defined(_OPENMP)
#  error "either enable OpenMP in the compiler settings or don't set ENABLE_OPENMP to 1"
# endif
#else	// no user preference; default to compiler setting
# ifdef _OPENMP
#  define ENABLE_OPENMP 1
# else
#  define ENABLE_OPENMP 0
# endif
#endif

#if ENABLE_OPENMP
# include <omp.h>
#else
# define omp_get_num_threads() 1
# define omp_get_thread_num() 0
# define omp_in_parallel() 0
#endif

// wrapper macro that evaluates to nothing if !ENABLE_OPENMP
// (much more convenient than individual #if ENABLE_OPENMP)
#if ENABLE_OPENMP
# if MSC_VERSION
#  define OMP(args) __pragma(omp args)
# elif GCC_VERSION
#  define OMP _Pragma("omp " #args)
# else
#  error "port"
# endif
#else
# define OMP(args)
#endif

#endif	// #ifndef INCLUDED_EXTERNAL_LIBRARIES_OPENMP
