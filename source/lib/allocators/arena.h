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
 * arena allocator (variable-size blocks, no deallocation).
 */

#ifndef INCLUDED_ALLOCATORS_ARENA
#define INCLUDED_ALLOCATORS_ARENA

#include "lib/allocators/allocator_policies.h"

namespace Allocators {

/**
 * allocator design parameters:
 * - O(1) allocation;
 * - variable-size blocks;
 * - support for deallocating all objects;
 * - consecutive allocations are back-to-back;
 * - no extra alignment nor padding.
 **/
template<class Storage>
class Arena
{
public:
	Arena(size_t maxSize)
		: storage(maxSize)
	{
		DeallocateAll();
	}

	size_t RemainingBytes() const
	{
		return storage.MaxCapacity() - end;
	}

	void* Allocate(size_t size)
	{
		return (void*)StorageAppend(storage, end, size);
	}

	void DeallocateAll()
	{
		end = 0;
	}

	// @return whether the address lies within the previously allocated range.
	bool Contains(uintptr_t address) const
	{
		return (address - storage.Address()) < end;
	}

private:
	Storage storage;
	size_t end;
};

LIB_API void TestArena();

}	// namespace Allocators

#endif	// #ifndef INCLUDED_ALLOCATORS_ARENA
