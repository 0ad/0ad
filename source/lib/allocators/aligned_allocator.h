/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : aligned_allocator.h
 * Project     : 0 A.D.
 * Description : STL allocator for aligned memory
 * =========================================================================
 */

#ifndef ALIGNED_ALLOCATOR
#define ALIGNED_ALLOCATOR

#include "lib/bits.h"	// round_up
#include "lib/sysdep/arch/x86_x64/x86_x64.h"	// x86_x64_L1CacheLineSize
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

	AlignedAllocator() throw()
	{
	}

	AlignedAllocator(const AlignedAllocator&) throw()
	{
	}

	template <class U>
	AlignedAllocator (const AlignedAllocator<U>&) throw()
	{
	}

	~AlignedAllocator() throw()
	{
	}

	size_type max_size() const throw()
	{
		// maximum number of *elements* that can be allocated
		return std::numeric_limits<std::size_t>::max() / sizeof(T);
	}

	// allocate uninitialized storage
	pointer allocate(size_type numElements, const void* hint = 0)
	{
		const size_type alignment = x86_x64_L1CacheLineSize();
		const size_type elementSize = round_up(sizeof(T), alignment);
		const size_type size = numElements * elementSize;
		pointer p = (pointer)rtl_AllocateAligned(size, alignment);
		return p;
	}

	// deallocate storage of elements that have been destroyed
	void deallocate(pointer p, size_type num)
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
	}
};

// indicate that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator==(const AlignedAllocator<T1>&, const AlignedAllocator<T2>&) throw()
{
	return true;
}

template <class T1, class T2>
bool operator!=(const AlignedAllocator<T1>&, const AlignedAllocator<T2>&) throw()
{
	return false;
}

#endif	// #ifndef ALIGNED_ALLOCATOR
