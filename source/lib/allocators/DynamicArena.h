/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_ALLOCATORS_DYNAMIC_ARENA
#define INCLUDED_ALLOCATORS_DYNAMIC_ARENA

#include "lib/bits.h"

#include <new> // bad_alloc
#include <memory> // unique_ptr
#include <vector>

namespace Allocators {

/**
 * 'Blind' memory allocator. Characteristics:
 * - grows dynamically, and linearily, by BLOCK_SIZE
 * - no reallocations, pointers remain valid
 * - can allocate objects up to BLOCK_SIZE in size
 * - can handle any aligment (up to BLOCK_SIZE)
 * - Doesn't de-allocate unless cleared or destroyed.
 */
template<size_t BLOCK_SIZE>
class DynamicArena
{
protected:
	struct Block
	{
		bool Available(size_t n, size_t alignment) const
		{
			return n + ROUND_UP(size, alignment) <= BLOCK_SIZE;
		}

		uint8_t* Allocate(size_t n, size_t alignment)
		{
			size = ROUND_UP(size, alignment);
			uint8_t* ptr = &block[size];
			size += n;
			return ptr;
		}

		alignas(8) uint8_t block[BLOCK_SIZE];
		size_t size = 0;
	};

	NONCOPYABLE(DynamicArena);
public:
	DynamicArena()
	{
		m_Blocks.reserve(16);
		AllocateNewBlock();
	};

	void AllocateNewBlock()
	{
		m_Blocks.emplace_back(std::make_unique<Block>());
		m_NewestBlock = m_Blocks.back().get();
	}

	void* allocate(size_t n, const void*, size_t alignment)
	{
		// Safely handle zero-sized allocations (happens with GCC STL - see ticket #909).
		if (n == 0)
			n = 1;

		if (n > BLOCK_SIZE)
		{
			debug_warn("DynamicArena cannot allocate more than chunk size");
			throw std::bad_alloc();
		}

		if (!m_NewestBlock->Available(n, alignment))
			AllocateNewBlock();

		return reinterpret_cast<void*>(m_NewestBlock->Allocate(n, alignment));
	}

	void deallocate(void* UNUSED(p), size_t UNUSED(n))
	{
		// ignored
	}

	void clear()
	{
		m_NewestBlock = nullptr;
		m_Blocks.clear();
		AllocateNewBlock();
	}

protected:
	std::vector<std::unique_ptr<Block>> m_Blocks;
	Block* m_NewestBlock = nullptr;
};

} // namespace Allocators

#endif // INCLUDED_ALLOCATORS_DYNAMIC_ARENA
