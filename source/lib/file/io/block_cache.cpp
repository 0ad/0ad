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
 * cache for aligned I/O blocks.
 */

#include "precompiled.h"
#include "block_cache.h"

#include "lib/config2.h"	// CONFIG2_CACHE_READ_ONLY
#include "lib/file/common/file_stats.h"
#include "lib/lockfree.h"
#include "lib/allocators/pool.h"
#include "lib/fnv_hash.h"
#include "io_align.h"


//-----------------------------------------------------------------------------

BlockId::BlockId()
	: m_id(0)
{
}

BlockId::BlockId(const fs::wpath& pathname, off_t ofs)
{
	m_id = fnv_hash64(pathname.string().c_str(), pathname.string().length()*sizeof(pathname.string()[0]));
	const size_t indexBits = 16;
	m_id <<= indexBits;
	const off_t blockIndex = off_t(ofs / BLOCK_SIZE);
	debug_assert(blockIndex < off_t(1) << indexBits);
	m_id |= blockIndex;
}

bool BlockId::operator==(const BlockId& rhs) const
{
	return m_id == rhs.m_id;
}

bool BlockId::operator!=(const BlockId& rhs) const
{
	return !operator==(rhs);
}


//-----------------------------------------------------------------------------

struct Block
{
	Block(BlockId id, const shared_ptr<u8>& buf)
	{
		this->id = id;
		this->buf = buf;
	}

	// block is "valid" and can satisfy Retrieve() requests if a
	// (non-default-constructed) ID has been assigned.
	BlockId id;

	// this block is "in use" if use_count != 1.
	shared_ptr<u8> buf;
};


//-----------------------------------------------------------------------------

class BlockCache::Impl
{
public:
	Impl(size_t numBlocks)
		: m_maxBlocks(numBlocks)
	{
	}

	void Add(BlockId id, const shared_ptr<u8>& buf)
	{
		if(m_blocks.size() > m_maxBlocks)
		{
#if CONFIG2_CACHE_READ_ONLY
			mprotect((void*)m_blocks.front().buf.get(), BLOCK_SIZE, PROT_READ);
#endif
			m_blocks.pop_front();	// evict oldest block
		}

#if CONFIG2_CACHE_READ_ONLY
		mprotect((void*)buf.get(), BLOCK_SIZE, PROT_WRITE|PROT_READ);
#endif
		m_blocks.push_back(Block(id, buf));
	}

	bool Retrieve(BlockId id, shared_ptr<u8>& buf)
	{
		// (linear search is ok since we only expect to manage a few blocks)
		for(size_t i = 0; i < m_blocks.size(); i++)
		{
			Block& block = m_blocks[i];
			if(block.id == id)
			{
				buf = block.buf;
				return true;
			}
		}

		return false;
	}

	void InvalidateAll()
	{
		// note: don't check whether any references are held etc. because
		// this should only be called at the end of the (test) program.
		m_blocks.clear();
	}

private:
	size_t m_maxBlocks;
	typedef std::deque<Block> Blocks;
	Blocks m_blocks;
};


//-----------------------------------------------------------------------------

BlockCache::BlockCache(size_t numBlocks)
	: impl(new Impl(numBlocks))
{
}

void BlockCache::Add(BlockId id, const shared_ptr<u8>& buf)
{
	impl->Add(id, buf);
}

bool BlockCache::Retrieve(BlockId id, shared_ptr<u8>& buf)
{
	return impl->Retrieve(id, buf);
}

void BlockCache::InvalidateAll()
{
	return impl->InvalidateAll();
}
