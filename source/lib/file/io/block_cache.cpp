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

BlockId::BlockId(const Path& pathname, off_t ofs)
{
	m_id = fnv_hash64(pathname.string().c_str(), pathname.string().length());
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
