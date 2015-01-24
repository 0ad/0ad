#ifndef INCLUDED_ALIGNMENT
#define INCLUDED_ALIGNMENT

#include "lib/sysdep/compiler.h"	// MSC_VERSION
#include "lib/sysdep/arch.h"	// ARCH_AMD64

template<typename T>
inline bool IsAligned(T t, uintptr_t multiple)
{
	return (uintptr_t(t) % multiple) == 0;
}

template<size_t multiple>
inline size_t Align(size_t n)
{
	cassert(multiple != 0 && ((multiple & (multiple-1)) == 0));	// is power of 2
	return (n + multiple-1) & ~(multiple-1);
}


// bridge the differences between MSC and GCC alignment definitions.
// example: ALIGNED(int, 8) myAlignedVariable = 0;
#if MSC_VERSION
# define ALIGNED(type, multiple) __declspec(align(multiple)) type
#elif GCC_VERSION
# define ALIGNED(type, multiple) type __attribute__((aligned(multiple)))
#else
# define ALIGNED(type, multiple) type
#endif


//
// SIMD vector
//

static const size_t vectorSize = 16;
#define VECTOR_ALIGNED(type) ALIGNED(type, 16)	// ALIGNED() requires a literal; keep in sync with vectorSize

#define ASSERT_VECTOR_MULTIPLE(size)\
	ASSERT(IsAligned(size, vectorSize))

#define ASSERT_VECTOR_ALIGNED(pointer)\
	ASSERT_VECTOR_MULTIPLE(pointer);\
	ASSUME_ALIGNED(pointer, vectorSize)


//
// CPU cache
//

static const size_t cacheLineSize = 64;	// (L2)
#define CACHE_ALIGNED(type) ALIGNED(type, 64)	// ALIGNED() requires a literal; keep in sync with cacheLineSize




//
// MMU pages
//

static const size_t pageSize = 0x1000;	// 4 KB
static const size_t largePageSize = 0x200000;	// 2 MB


//
// misc
//

static const size_t allocationAlignment = 16;

static const size_t KiB = size_t(1) << 10;
static const size_t MiB = size_t(1) << 20;
static const size_t GiB = size_t(1) << 30;

// waio opens files with FILE_FLAG_NO_BUFFERING, so Windows requires
// file offsets / buffers and sizes to be sector-aligned. querying the
// actual sector size via GetDiskFreeSpace is inconvenient and slow.
// we always request large blocks anyway, so just check whether inputs
// are aligned to a `maximum' sector size. this catches common mistakes
// before they cause scary "IO failed" errors. if the value turns out
// to be too low, the Windows APIs will still complain.
static const uintptr_t maxSectorSize = 0x1000;

#endif	// #ifndef INCLUDED_ALIGNMENT
