/* Copyright (C) 2010 Wildfire Games.
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
 * STL allocator for aligned memory
 */

#ifndef ALIGNED_ALLOCATOR
#define ALIGNED_ALLOCATOR

#include "lib/bits.h"	// round_up
#include "lib/sysdep/arch/x86_x64/cache.h"
#include "lib/sysdep/rtl.h"	// rtl_AllocateAligned


/**
 * stateless STL allocator that aligns elements to the L1 cache line size.
 *
 * note: the alignment is hard-coded to avoid any allocator state.
 * this avoids portability problems, which is important since allocators
 * are rather poorly specified.
 *
 * references:
 * http://www.tantalon.com/pete/customallocators.ppt
 * http://www.flipcode.com/archives/Aligned_Block_Allocation.shtml
 * http://www.josuttis.com/cppcode/allocator.html
 *
 * derived from code that bears the following copyright notice:
 * (C) Copyright Nicolai M. Josuttis 1999.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 **/
template<class T>
class AlignedAllocator
{
public:
	// type definitions
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	// rebind allocator to type U
	template <class U>
	struct rebind
	{
		typedef AlignedAllocator<U> other;
	};

	pointer address(reference value) const
	{
		return &value;
	}

	const_pointer address(const_reference value) const
	{
		return &value;
	}

	NOTHROW_DEFINE AlignedAllocator()
	{
	}

	NOTHROW_DEFINE AlignedAllocator(const AlignedAllocator&)
	{
	}

	template <class U>
	NOTHROW_DEFINE AlignedAllocator (const AlignedAllocator<U>&)
	{
	}

	NOTHROW_DEFINE ~AlignedAllocator()
	{
	}

	NOTHROW_DEFINE size_type max_size() const
	{
		// maximum number of *elements* that can be allocated
		return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	// allocate uninitialized storage
	pointer allocate(size_type numElements)
	{
		const size_type alignment = x86_x64::Caches(x86_x64::L1D)->entrySize;
		const size_type elementSize = round_up(sizeof(T), alignment);
		const size_type size = numElements * elementSize;
		pointer p = (pointer)rtl_AllocateAligned(size, alignment);
		return p;
	}

	// deallocate storage of elements that have been destroyed
	void deallocate(pointer p, size_type UNUSED(num))
	{
		rtl_FreeAligned((void*)p);
	}

	void construct(pointer p, const T& value)
	{
		new((void*)p) T(value);
	}

	void destroy(pointer p)
	{
		p->~T();
		UNUSED2(p);	// otherwise, warning is raised for reasons unknown
	}
};

// indicate that all specializations of this allocator are interchangeable
template <class T1, class T2>
NOTHROW_DEFINE bool operator==(const AlignedAllocator<T1>&, const AlignedAllocator<T2>&)
{
	return true;
}

template <class T1, class T2>
NOTHROW_DEFINE bool operator!=(const AlignedAllocator<T1>&, const AlignedAllocator<T2>&)
{
	return false;
}

#endif	// #ifndef ALIGNED_ALLOCATOR
