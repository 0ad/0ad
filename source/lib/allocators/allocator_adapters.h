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

template<size_t commitSize = largePageSize, vm::PageType pageType = vm::kDefault, int prot = PROT_READ|PROT_WRITE>
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

#endif	// #ifndef ALLOCATOR_ADAPTERS
