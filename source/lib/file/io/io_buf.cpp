#include "precompiled.h"
#include "io_buf.h"

#include "lib/allocators/allocators.h"	// AllocatorChecker

#ifndef NDEBUG
static AllocatorChecker allocatorChecker;
#endif

IoBuf io_buf_Allocate(size_t size)
{
	void* p = page_aligned_alloc(size);
	if(!p)
		throw std::bad_alloc();
#ifndef NDEBUG
	allocatorChecker.notify_alloc(p, size);
#endif
	return (IoBuf)p;
}

void io_buf_Deallocate(IoBuf buf, size_t size)
{
	void* p = (void*)buf;
#ifndef NDEBUG
	allocatorChecker.notify_free(p, size);
#endif
	page_aligned_free(p, size);
}
