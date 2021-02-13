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

#include <memory> // unique_ptr
#include <new> // bad_alloc
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
	class Block
	{
	public:
		bool Available(size_t n, size_t alignment) const
		{
			return n + ROUND_UP(m_Size, alignment) <= BLOCK_SIZE;
		}

		uint8_t* Allocate(size_t n, size_t alignment)
		{
			m_Size = ROUND_UP(m_Size, alignment);
			uint8_t* ptr = &m_Data[m_Size];
			m_Size += n;
			return ptr;
		}

	private:
		size_t m_Size = 0;
		std::unique_ptr<uint8_t[]> m_Data = std::make_unique<uint8_t[]>(BLOCK_SIZE);
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
		m_Blocks.emplace_back();
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

		if (!m_Blocks.back().Available(n, alignment))
			AllocateNewBlock();

		return reinterpret_cast<void*>(m_Blocks.back().Allocate(n, alignment));
	}

	void deallocate(void* UNUSED(p), size_t UNUSED(n))
	{
		// ignored
	}

	void clear()
	{
		m_Blocks.clear();
		AllocateNewBlock();
	}

protected:
	std::vector<Block> m_Blocks;
};

} // namespace Allocators

#endif // INCLUDED_ALLOCATORS_DYNAMIC_ARENA
