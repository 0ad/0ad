/**
 * =========================================================================
 * File        : wposix_types.h
 * Project     : 0 A.D.
 * Description : type definitions needed for wposix.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WPOSIX_TYPES
#define INCLUDED_WPOSIX_TYPES

//
// <inttypes.h>
//

typedef signed char int8_t;
typedef short int16_t;

// already defined by MinGW
#if MSC_VERSION
typedef int int32_t;
#endif

#if MSC_VERSION || ICC_VERSION || LCC_VERSION
typedef __int64 int64_t;
#else
typedef long long int64_t;
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#if MSC_VERSION || ICC_VERSION || LCC_VERSION
typedef unsigned __int64 uint64_t;
#else
typedef unsigned long long uint64_t;
#endif

// note: we used to define [u]intptr_t here (if not already done).
// however, it's not necessary with VC7 and later, and the compiler's
// definition squelches 'pointer-to-int truncation' warnings, so don't.

#define INT16_MIN -32768
#define INT16_MAX 32767
#define UINT16_MIN 0u
#define UINT16_MAX 65536u


//
// <sys/types.h>
//

typedef long ssize_t;
// prevent wxWidgets from (incompatibly) redefining it
#define HAVE_SSIZE_T
typedef long off_t;


//
// <limits.h>
//

// Win32 MAX_PATH is 260; our number may be a bit more efficient.
#define PATH_MAX 256u

#if OS_WIN
# ifndef SIZE_MAX // VC2005 already defines this in limits.h
#  define SIZE_MAX 0xFFFFFFFF
# endif
#else
# define SIZE_MAX 0xFFFFFFFFFFFFFFFF
#endif


#endif	// #ifndef INCLUDED_WPOSIX_TYPES
