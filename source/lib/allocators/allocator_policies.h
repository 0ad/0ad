/* Copyright (c) 2011 Wildfire Games
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
 * policy class templates for allocators.
 */

#ifndef ALLOCATOR_POLICIES
#define ALLOCATOR_POLICIES

#include "lib/alignment.h"	// pageSize
#include "lib/allocators/allocator_adapters.h"
#include "lib/allocators/freelist.h"


namespace Allocators {


//-----------------------------------------------------------------------------
// Growth

// O(N) allocations, O(1) wasted space.
template<size_t increment = pageSize>
struct Growth_Linear
{
	size_t operator()(size_t oldSize) const
	{
		return oldSize + increment;
	}
};

// O(log r) allocations, O(N) wasted space. NB: the common choice of
// expansion factor r = 2 (e.g. in the GCC STL) prevents
// Storage_Reallocate from reusing previous memory blocks,
// thus constantly growing the heap and decreasing locality.
// Alexandrescu [C++ and Beyond 2010] recommends r < 33/25.
// we approximate this with a power of two divisor to allow shifting.
// C++ does allow reference-to-float template parameters, but
// integer arithmetic is expected to be faster.
// (Storage_Commit should use 2:1 because it is cheaper to
// compute and retains power-of-two sizes.)
template<size_t multiplier = 21, size_t divisor = 16>
struct Growth_Exponential
{
	size_t operator()(size_t oldSize) const
	{
		const size_t product = oldSize * multiplier;

		// detect overflow, but allow equality in case oldSize = 0,
		// which isn't a problem because Storage_Commit::Expand
		// raises it to requiredCapacity.
		ASSERT(product >= oldSize);

		return product / divisor;
	}
};


//-----------------------------------------------------------------------------
// Storage

// a contiguous region of memory (not just an "array", because
// allocators such as Arena append variable-sized intervals).
//
// we don't store smart pointers because storage usually doesn't need
// to be copied, and ICC 11 sometimes wasn't able to inline Address().
struct Storage
{
	// @return starting address (alignment depends on the allocator).
	uintptr_t Address() const;

	// @return size [bytes] of currently accessible memory.
	size_t Capacity() const;

	// @return largest possible capacity [bytes].
	size_t MaxCapacity() const;

	// expand Capacity() to at least requiredCapacity (possibly more
	//   depending on GrowthPolicy).
	// @param requiredCapacity > Capacity()
	// @return false and leave Capacity() unchanged if expansion failed,
	//   which is guaranteed to happen if requiredCapacity > MaxCapacity().
	bool Expand(size_t requiredCapacity);
};


// allocate once and refuse subsequent expansion.
template<class Allocator = Allocator_Aligned<> >
class Storage_Fixed
{
	NONCOPYABLE(Storage_Fixed);
public:
	Storage_Fixed(size_t size)
		: maxCapacity(size)
		, storage(allocator.allocate(maxCapacity))
	{
	}

	~Storage_Fixed()
	{
		allocator.deallocate(storage, maxCapacity);
	}

	uintptr_t Address() const
	{
		return uintptr_t(storage);
	}

	size_t Capacity() const
	{
		return maxCapacity;
	}

	size_t MaxCapacity() const
	{
		return maxCapacity;
	}

	bool Expand(size_t UNUSED(requiredCapacity))
	{
		return false;
	}

private:
	Allocator allocator;
	size_t maxCapacity;	// must be initialized before storage
	void* storage;
};


// unlimited expansion by allocating larger storage and copying.
// (basically equivalent to std::vector, although Growth_Exponential
// is much more cache and allocator-friendly than the GCC STL)
template<class Allocator = Allocator_Heap, class GrowthPolicy = Growth_Exponential<> >
class Storage_Reallocate
{
	NONCOPYABLE(Storage_Reallocate);
public:
	Storage_Reallocate(size_t initialCapacity)
		: capacity(initialCapacity)
		, storage(allocator.allocate(initialCapacity))
	{
	}

	~Storage_Reallocate()
	{
		allocator.deallocate(storage, capacity);
	}

	uintptr_t Address() const
	{
		return uintptr_t(storage);
	}

	size_t Capacity() const
	{
		return capacity;
	}

	size_t MaxCapacity() const
	{
		return std::numeric_limits<size_t>::max();
	}

	bool Expand(size_t requiredCapacity)
	{
		size_t newCapacity = std::max(requiredCapacity, GrowthPolicy()(capacity));
		void* newStorage = allocator.allocate(newCapacity);
		if(!newStorage)
			return false;
		memcpy(newStorage, storage, capacity);
		std::swap(capacity, newCapacity);
		std::swap(storage, newStorage);
		allocator.deallocate(newStorage, newCapacity);	// free PREVIOUS storage
		return true;
	}

private:
	Allocator allocator;
	size_t capacity;	// must be initialized before storage
	void* storage;
};


// expand up to the limit of the allocated address space by
// committing physical memory. this avoids copying and
// reduces wasted physical memory.
template<class Allocator = Allocator_AddressSpace<>, class GrowthPolicy = Growth_Exponential<2,1> >
class Storage_Commit
{
	NONCOPYABLE(Storage_Commit);
public:
	Storage_Commit(size_t maxCapacity_)
		: maxCapacity(Align<pageSize>(maxCapacity_))	// see Expand
		, storage(allocator.allocate(maxCapacity))
		, capacity(0)
	{
	}

	~Storage_Commit()
	{
		allocator.deallocate(storage, maxCapacity);
	}

	uintptr_t Address() const
	{
		return uintptr_t(storage);
	}

	size_t Capacity() const
	{
		return capacity;
	}

	size_t MaxCapacity() const
	{
		return maxCapacity;
	}

	bool Expand(size_t requiredCapacity)
	{
		size_t newCapacity = std::max(requiredCapacity, GrowthPolicy()(capacity));
		// reduce the number of expensive commits by accurately
		// reflecting the actual capacity. this is safe because
		// we also round up maxCapacity.
		newCapacity = Align<pageSize>(newCapacity);
		if(newCapacity > maxCapacity)
			return false;
		if(!vm::Commit(Address()+capacity, newCapacity-capacity))
			return false;
		capacity = newCapacity;
		return true;
	}

private:
	Allocator allocator;
	size_t maxCapacity;	// must be initialized before storage
	void* storage;
	size_t capacity;
};


// implicitly expand up to the limit of the allocated address space by
// committing physical memory when a page is first accessed.
// this is basically equivalent to Storage_Commit with Growth_Linear,
// except that there is no need to call Expand.
template<class Allocator = Allocator_AddressSpace<> >
class Storage_AutoCommit
{
	NONCOPYABLE(Storage_AutoCommit);
public:
	Storage_AutoCommit(size_t maxCapacity_)
		: maxCapacity(Align<pageSize>(maxCapacity_))	// match user's expectation
		, storage(allocator.allocate(maxCapacity))
	{
		vm::BeginOnDemandCommits();
	}

	~Storage_AutoCommit()
	{
		vm::EndOnDemandCommits();
		allocator.deallocate(storage, maxCapacity);
	}

	uintptr_t Address() const
	{
		return uintptr_t(storage);
	}

	size_t Capacity() const
	{
		return maxCapacity;
	}

	size_t MaxCapacity() const
	{
		return maxCapacity;
	}

	bool Expand(size_t UNUSED(requiredCapacity))
	{
		return false;
	}

private:
	Allocator allocator;
	size_t maxCapacity;	// must be initialized before storage
	void* storage;
};


// reserve and return a pointer to space at the end of storage,
// expanding it if need be.
// @param end total number of previously reserved bytes; will be
//   increased by size if the allocation succeeds.
// @param size [bytes] to reserve.
// @return address of allocated space, or 0 if storage is full
//   and cannot expand any further.
template<class Storage>
static inline uintptr_t StorageAppend(Storage& storage, size_t& end, size_t size)
{
	size_t newEnd = end + size;
	if(newEnd > storage.Capacity())
	{
		if(!storage.Expand(newEnd))	// NB: may change storage.Address()
			return 0;
	}

	std::swap(end, newEnd);
	return storage.Address() + newEnd;
}


// invoke operator() on default-constructed instantiations of
// Functor for reasonable combinations of Storage and their parameters.
template<template<class Storage> class Functor>
static void ForEachStorage()
{
	Functor<Storage_Fixed<Allocator_Heap> >()();
	Functor<Storage_Fixed<Allocator_Aligned<> > >()();

	Functor<Storage_Reallocate<Allocator_Heap, Growth_Linear<> > >()();
	Functor<Storage_Reallocate<Allocator_Heap, Growth_Exponential<> > >()();
	Functor<Storage_Reallocate<Allocator_Aligned<>, Growth_Linear<> > >()();
	Functor<Storage_Reallocate<Allocator_Aligned<>, Growth_Exponential<> > >()();

	Functor<Storage_Commit<Allocator_AddressSpace<>, Growth_Linear<> > >()();
	Functor<Storage_Commit<Allocator_AddressSpace<>, Growth_Exponential<> > >()();

	Functor<Storage_AutoCommit<> >()();
}

}	// namespace Allocators

#endif	// #ifndef ALLOCATOR_POLICIES
