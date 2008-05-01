/**
 * =========================================================================
 * File        : pool.h
 * Project     : 0 A.D.
 * Description : pool allocator
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_POOL
#define INCLUDED_POOL

#include "dynarray.h"

/**
 * allocator design parameters:
 * - O(1) alloc and free;
 * - either fixed- or variable-sized blocks;
 * - doesn't preallocate the entire pool;
 * - returns sequential addresses.
 *
 * opaque! do not read/write any fields!
 **/
struct Pool
{
	DynArray da;

	/**
	 * size of elements. = 0 if pool set up for variable-sized
	 * elements, otherwise rounded up to pool alignment.
	 **/
	size_t el_size;

	/**
	 * pointer to freelist (opaque); see freelist_*.
	 * never used (remains 0) if elements are of variable size.
	 **/
	void* freelist;
};

/**
 * pass as pool_create's <el_size> param to indicate variable-sized allocs
 * are required (see below).
 **/
const size_t POOL_VARIABLE_ALLOCS = ~(size_t)0u;

/**
 * Ready Pool for use.
 *
 * @param Pool*
 * @param max_size Max size [bytes] of the Pool; this much
 * (rounded up to next page multiple) virtual address space is reserved.
 * no virtual memory is actually committed until calls to pool_alloc.
 * @param el_size Number of bytes that will be returned by each
 * pool_alloc (whose size parameter is then ignored). Can be 0 to
 * allow variable-sized allocations, but pool_free is then unusable.
 * @return LibError
 **/
LIB_API LibError pool_create(Pool* p, size_t max_size, size_t el_size);

/**
 * free all memory (address space + physical) that constitutes the
 * given Pool.
 *
 * future alloc and free calls on this pool will fail.
 * continued use of the allocated memory (*) is
 * impossible because it is marked not-present via MMU.
 * (* no matter if in freelist or unused or "allocated" to user)
 *
 * @param Pool*
 * @return LibError.
 **/
LIB_API LibError pool_destroy(Pool* p);

/**
 * indicate whether a pointer was allocated from the given pool.
 *
 * this is useful for callers that use several types of allocators.
 *
 * @param Pool*
 * @return bool.
 **/
LIB_API bool pool_contains(const Pool* p, void* el);

/**
 * Dole out memory from the pool.
 * exhausts the freelist before returning new entries to improve locality.
 *
 * @param Pool*
 * @param size bytes to allocate; ignored if pool_create's el_size was not 0.
 * @return allocated memory, or 0 if the Pool would have to be expanded and
 * there isn't enough memory to do so.
 **/
LIB_API void* pool_alloc(Pool* p, size_t size);

/**
 * Make a fixed-size element available for reuse in the given Pool.
 *
 * this is not allowed if the Pool was created for variable-size elements.
 * rationale: avoids having to pass el_size here and compare with size when
 * allocating; also prevents fragmentation and leaking memory.
 *
 * @param Pool*
 * @param el Element returned by pool_alloc.
 **/
LIB_API void pool_free(Pool* p, void* el);

/**
 * "free" all user allocations that ensued from the given Pool.
 *
 * this resets it as if freshly pool_create-d, but doesn't release the
 * underlying reserved virtual memory.
 *
 * @param Pool*
 **/
LIB_API void pool_free_all(Pool* p);


#ifdef __cplusplus

/**
 * C++ wrapper on top of pool_alloc that's slightly easier to use.
 *
 * T must be POD (Plain Old Data) because it is memset to 0!
 **/
template<class T>
class PoolAllocator
{
public:
	explicit PoolAllocator(size_t maxElements)
	{
		(void)pool_create(&m_pool, maxElements*sizeof(T), sizeof(T));
	}

	~PoolAllocator()
	{
		(void)pool_destroy(&m_pool);
	}

	T* AllocateZeroedMemory()
	{
		T* t = (T*)pool_alloc(&m_pool, 0);
		if(!t)
			throw std::bad_alloc();
		memset(t, 0, sizeof(T));
		return t;
	}

	void Free(T* t)
	{
		pool_free(&m_pool, t);
	}

private:
	Pool m_pool;
};

#endif

#endif	// #ifndef INCLUDED_POOL
