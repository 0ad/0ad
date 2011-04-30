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
	cassert_dependent(multiple != 0 && ((multiple & (multiple-1)) == 0));	// is power of 2
	return (n + multiple-1) & ~(multiple-1);
}


//
// SIMD vector
//

static const size_t vectorSize = 16;

#define VERIFY_VECTOR_MULTIPLE(size)\
	VERIFY(IsAligned(size, vectorSize))

#define VERIFY_VECTOR_ALIGNED(pointer)\
	VERIFY_VECTOR_MULTIPLE(pointer);\
	ASSUME_ALIGNED(pointer, vectorSize)


//
// CPU cache
//

static const size_t cacheLineSize = 64;	// (L2)

#if MSC_VERSION
#define CACHE_ALIGNED __declspec(align(64))	// align() requires a literal; keep in sync with cacheLineSize
#endif


//
// MMU pages
//

static const size_t pageSize = 0x1000;	// 4 KB
static const size_t largePageSize = 0x200000;	// 2 MB


//
// misc
//

static const size_t allocationAlignment = ARCH_AMD64? 16 : 8;

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
