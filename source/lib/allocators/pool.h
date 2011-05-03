/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * pool allocator
 */

#ifndef INCLUDED_ALLOCATORS_POOL
#define INCLUDED_ALLOCATORS_POOL

#include "lib/allocators/dynarray.h"

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
 * pass as pool_create's \<el_size\> param to indicate variable-sized allocs
 * are required (see below).
 **/
const size_t POOL_VARIABLE_ALLOCS = ~(size_t)0u;

/**
 * Ready Pool for use.
 *
 * @param p Pool*
 * @param max_size Max size [bytes] of the Pool; this much
 * (rounded up to next page multiple) virtual address space is reserved.
 * no virtual memory is actually committed until calls to pool_alloc.
 * @param el_size Number of bytes that will be returned by each
 * pool_alloc (whose size parameter is then ignored). Can be 0 to
 * allow variable-sized allocations, but pool_free is then unusable.
 * @return Status
 **/
LIB_API Status pool_create(Pool* p, size_t max_size, size_t el_size);

/**
 * free all memory (address space + physical) that constitutes the
 * given Pool.
 *
 * future alloc and free calls on this pool will fail.
 * continued use of the allocated memory (*) is
 * impossible because it is marked not-present via MMU.
 * (* no matter if in freelist or unused or "allocated" to user)
 *
 * @param p Pool*
 * @return Status.
 **/
LIB_API Status pool_destroy(Pool* p);

/**
 * indicate whether a pointer was allocated from the given pool.
 *
 * this is useful for callers that use several types of allocators.
 *
 * @param p Pool*
 * @param el
 * @return bool.
 **/
LIB_API bool pool_contains(const Pool* p, void* el);

/**
 * Dole out memory from the pool.
 * exhausts the freelist before returning new entries to improve locality.
 *
 * @param p Pool*
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
 * @param p Pool*
 * @param el Element returned by pool_alloc.
 **/
LIB_API void pool_free(Pool* p, void* el);

/**
 * "free" all user allocations that ensued from the given Pool.
 *
 * this resets it as if freshly pool_create-d, but doesn't release the
 * underlying reserved virtual memory.
 *
 * @param p Pool*
 **/
LIB_API void pool_free_all(Pool* p);

/**
 * Return the number of bytes committed in the pool's backing array.
 *
 * This is roughly the number of bytes allocated in this pool plus the
 * unused freelist entries.
 *
 * @param p Pool*
 * @return number of bytes
 **/
LIB_API size_t pool_committed(Pool* p);


/**
 * C++ wrapper on top of pool_alloc for fixed-size allocations (determined by sizeof(T))
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

/**
 * C++ wrapper on top of pool_alloc for variable-sized allocations.
 * Memory is returned uninitialised.
 */
class RawPoolAllocator
{
public:
	/**
	 * @param maxSize maximum size of pool in bytes
	 */
	explicit RawPoolAllocator(size_t maxSize)
	{
		const size_t el_size = 0;	// sizes will be passed to each pool_alloc
		(void)pool_create(&m_pool, maxSize, el_size);
	}

	~RawPoolAllocator()
	{
		(void)pool_destroy(&m_pool);
	}

	/**
	 * @param count number of elements of type T to allocate space for
	 */
	template<typename T>
	T* AllocateMemory(size_t count)
	{
		T* t = (T*)pool_alloc(&m_pool, count*sizeof(T));
		if(!t)
			throw std::bad_alloc();
		return t;
	}

	size_t GetCommittedSize()
	{
		return pool_committed(&m_pool);
	}

private:
	Pool m_pool;
};

/**
 * STL-compatible allocator based on a RawPoolAllocator.
 * (Allocated memory is never freed, until the RawPoolAllocator is destroyed.)
 */
template<typename T>
class pool_allocator
{
private:
	// No default constructor
	pool_allocator() throw ();

public:
	RawPoolAllocator& p;

	typedef T value_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	template<class U>
	struct rebind
	{
		typedef pool_allocator<U> other;
	};

	explicit pool_allocator(RawPoolAllocator& pool) throw () :
		p(pool)
	{
	}

	template<typename U>
	pool_allocator(const pool_allocator<U>& alloc) throw () :
		p(alloc.p)
	{
	}

	pool_allocator& operator=(const pool_allocator&) throw ()
	{
	}

	pointer address(reference r)
	{
		return &r;
	}

	const_pointer address(const_reference s)
	{
		return &s;
	}

	size_type max_size() const throw ()
	{
		return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	void construct(const pointer ptr, const value_type& t)
	{
		new (ptr) T(t);
	}

	void destroy(const pointer ptr)
	{
		ptr->~T();
		UNUSED2(ptr); // silence MSVC warnings
	}

	pointer allocate(size_type n)
	{
		return p.AllocateMemory<value_type> (n);
	}

	pointer allocate(size_type n, const void* const)
	{
		return allocate(n);
	}

	void deallocate(const pointer UNUSED(ptr), const size_type UNUSED(n))
	{
		// ignore deallocations
	}
};

template<class T1, class T2>
bool operator==(const pool_allocator<T1>&, const pool_allocator<T2>&) throw ()
{
	return true;
}

template<class T1, class T2>
bool operator!=(const pool_allocator<T1>&, const pool_allocator<T2>&) throw ()
{
	return false;
}

#endif	// #ifndef INCLUDED_ALLOCATORS_POOL
