#include "precompiled.h"
#include "lib/sysdep/rtl.h"

void* rtl_AllocateAligned(size_t size, size_t alignment)
{
	return _aligned_malloc(size, alignment);
}

void rtl_FreeAligned(void* alignedPointer)
{
	_aligned_free(alignedPointer);
}
