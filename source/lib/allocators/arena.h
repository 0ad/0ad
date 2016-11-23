/* Copyright (c) 2013 Wildfire Games
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
template<class Storage = Storage_Fixed<> >
class Arena
{
	NONCOPYABLE(Arena);
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

	void* allocate(size_t size)
	{
		return (void*)StorageAppend(storage, end, size);
	}

	void deallocate(void* UNUSED(p), size_t UNUSED(size))
	{
		// ignored
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


/**
 * allocator design parameters:
 * - grow dynamically with a fixed chunkSize
 * - for frequent allocations of size << chunkSize
 * - no reallocations, pointers remain valid
 **/
class DynamicArena
{
	struct ArenaChunk
	{
		bool Available(size_t size) const
		{
			return size <= (capacity - end);
		}

		// Must check Available first or this may return an invalid address
		uintptr_t Allocate(size_t size)
		{
			uintptr_t ptr = storage + end;
			end += size;
			return ptr;
		}

		uintptr_t storage;
		size_t end;
		size_t capacity;
		ArenaChunk* next;
	};

	NONCOPYABLE(DynamicArena);
public:
	DynamicArena(size_t chunkSize) : chunkSize(chunkSize), head(NULL)
	{
		AllocateNewChunk();
	}

	~DynamicArena()
	{
		ArenaChunk* chunk = head;
		while (chunk != NULL)
		{
			ArenaChunk* next = chunk->next;
			free(chunk);
			chunk = next;
		}
	}

	void AllocateNewChunk()
	{
		// For efficiency, do a single allocation with the ArenaChunk and its storage
		ArenaChunk* next = head;
		head = (ArenaChunk*)malloc(sizeof(ArenaChunk) + chunkSize);
		ENSURE(head);
		head->storage = sizeof(ArenaChunk) + uintptr_t(head);
		head->end = 0;
		head->capacity = chunkSize;
		head->next = next;
	}

	void* allocate(size_t size)
	{
		if (size > chunkSize)
		{
			debug_warn(L"DynamicArena cannot allocate more than chunk size");
			throw std::bad_alloc();
		}
		else if (!head->Available(size))
			AllocateNewChunk();

		return (void*)head->Allocate(size);
	}

	void deallocate(void* UNUSED(p), size_t UNUSED(size))
	{
		// ignored
	}

private:

	const size_t chunkSize;
	ArenaChunk *head;
};


}	// namespace Allocators

#endif	// #ifndef INCLUDED_ALLOCATORS_ARENA
