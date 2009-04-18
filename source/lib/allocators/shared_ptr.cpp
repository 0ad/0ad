#include "precompiled.h"
#include "shared_ptr.h"

#include "allocators.h"	// AllocatorChecker


#ifndef NDEBUG
static AllocatorChecker s_allocatorChecker;
#endif

class CheckedArrayDeleter
{
public:
	CheckedArrayDeleter(size_t size)
		: m_size(size)
	{
	}

	void operator()(u8* p)
	{
		debug_assert(m_size != 0);
#ifndef NDEBUG
		s_allocatorChecker.OnDeallocate(p, m_size);
#endif
		delete[] p;
		m_size = 0;
	}

private:
	size_t m_size;
};

shared_ptr<u8> Allocate(size_t size)
{
	debug_assert(size != 0);

	u8* p = new u8[size];
#ifndef NDEBUG
	s_allocatorChecker.OnAllocate(p, size);
#endif

	return shared_ptr<u8>(p, CheckedArrayDeleter(size));
}
