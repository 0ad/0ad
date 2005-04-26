
//
// <inttypes.h>
//

typedef signed char int8_t;
typedef short int16_t;

// already defined by MinGW
#ifdef _MSC_VER
typedef int int32_t;
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__LCC__)
typedef __int64 int64_t;
#elif defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_C) || defined(__DMC__)
typedef long long int64_t;
#else
#error "port int64_t"
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__LCC__)
typedef unsigned __int64 uint64_t;
#elif defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_C) || defined(__DMC__)
typedef unsigned long long uint64_t;
#else
#error "port uint64_t"
#endif

// note: we used to define [u]intptr_t here (if not already done).
// however, it's not necessary with VC7 and later, and the compiler's
// definition squelches 'pointer-to-int truncation' warnings, so don't.


//
// <sys/types.h>
//

typedef long ssize_t;