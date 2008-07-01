#include "precompiled.h"
#include "lib/sysdep/rtl.h"

void* rtl_AllocateAligned(size_t size, size_t alignment)
{
	void *ptr;
	int ret = posix_memalign(&ptr, size, alignment);
	if (ret) {
		// TODO: report error?
		return NULL;
	}
	return ptr;
}

void rtl_FreeAligned(void* alignedPointer)
{
	free(alignedPointer);
}
