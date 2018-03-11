/* Copyright (C) 2018 Wildfire Games.
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
 * adapters for allocators; provides a minimal subset of the
 * STL allocator interface.
 */

#ifndef ALLOCATOR_ADAPTERS
#define ALLOCATOR_ADAPTERS

#include <memory>

#include "lib/sysdep/rtl.h"
#include "lib/sysdep/vm.h"

// NB: STL allocators are parameterized on the object type and indicate
// the number of elements to [de]allocate. however, these adapters are
// only used for allocating storage and receive the number of bytes.

struct Allocator_Heap
{
	void* allocate(size_t size)
	{
		return malloc(size);
	}

	void deallocate(void* p, size_t UNUSED(size))
	{
		return free(p);
	}
};

template<size_t alignment = allocationAlignment>
struct Allocator_Aligned
{
	void* allocate(size_t size)
	{
		return rtl_AllocateAligned(size, alignment);
	}

	void deallocate(void* p, size_t UNUSED(size))
	{
		return rtl_FreeAligned(p);
	}
};

template<vm::PageType pageType = vm::kDefault, int prot = PROT_READ|PROT_WRITE>
struct Allocator_VM
{
	void* allocate(size_t size)
	{
		return vm::Allocate(size, pageType, prot);
	}

	void deallocate(void* p, size_t size)
	{
		vm::Free(p, size);
	}
};

template<size_t commitSize = g_LargePageSize, vm::PageType pageType = vm::kDefault, int prot = PROT_READ|PROT_WRITE>
struct Allocator_AddressSpace
{
	void* allocate(size_t size)
	{
		return vm::ReserveAddressSpace(size, commitSize, pageType, prot);
	}

	void deallocate(void* p, size_t size)
	{
		vm::ReleaseAddressSpace(p, size);
	}
};


/**
 * fully STL-compatible allocator that simply draws upon another Allocator.
 * this allows a single allocator to serve multiple STL containers.
 */
template<typename T, class Allocator>
class ProxyAllocator
{
public:
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
		typedef ProxyAllocator<U, Allocator> other;
	};

	// (required to be declared by boost::unordered_map, but should never be called)
	explicit NOTHROW_DEFINE ProxyAllocator();

	explicit NOTHROW_DEFINE ProxyAllocator(Allocator& allocator)
		: allocator(&allocator)
	{
	}

	template<typename U, class A>
	NOTHROW_DEFINE ProxyAllocator(const ProxyAllocator<U,A>& rhs)
		: allocator(rhs.allocator)
	{
	}

	// (required by VC2010 std::vector)
	bool operator==(const ProxyAllocator& rhs) const
	{
		return allocator == rhs.allocator;
	}
	bool operator!=(const ProxyAllocator& rhs) const
	{
		return !operator==(rhs);
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
		new(ptr) T(t);
	}

	void destroy(pointer ptr)
	{
		ptr->~T();
		UNUSED2(ptr); // silence MSVC warnings
	}

	pointer allocate(size_type n)
	{
		// safely handle zero-sized allocations (happens with GCC STL - see ticket #909).
		if(n == 0)
			n = 1;
		return (pointer)allocator->allocate(n*sizeof(T));
	}

	pointer allocate(size_type n, const void* const)
	{
		return allocate(n);
	}

	void deallocate(const pointer ptr, const size_type n)
	{
		return allocator->deallocate(ptr, n*sizeof(T));
	}

//private:	// otherwise copy ctor cannot access it
	Allocator* allocator;
};

#endif	// #ifndef ALLOCATOR_ADAPTERS
