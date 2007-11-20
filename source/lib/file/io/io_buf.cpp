#include "precompiled.h"
#include "io_buf.h"

#include "lib/allocators/allocators.h"	// AllocatorChecker
#include "lib/bits.h"	// round_up
#include "block_cache.h"	// BLOCK_SIZE


#ifndef NDEBUG
static AllocatorChecker allocatorChecker;
#endif


class IoBufDeleter
{
public:
	IoBufDeleter(size_t paddedSize)
		: m_paddedSize(paddedSize)
	{
		debug_assert(m_paddedSize != 0);
	}

	void operator()(const u8* mem)
	{
		debug_assert(m_paddedSize != 0);
#ifndef NDEBUG
		allocatorChecker.notify_free((void*)mem, m_paddedSize);
#endif
		page_aligned_free((void*)mem, m_paddedSize);
		m_paddedSize = 0;
	}

private:
	size_t m_paddedSize;
};


IoBuf io_buf_Allocate(size_t size)
{
	const size_t paddedSize = (size_t)round_up(size, BLOCK_SIZE);
	const u8* mem = (const u8*)page_aligned_alloc(paddedSize);
	if(!mem)
		throw std::bad_alloc();

#ifndef NDEBUG
	allocatorChecker.notify_alloc((void*)mem, paddedSize);
#endif

	return IoBuf(mem, IoBufDeleter(paddedSize));
}
