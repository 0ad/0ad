/**
 * =========================================================================
 * File        : block_cache.cpp
 * Project     : 0 A.D.
 * Description : cache for aligned I/O m_blocks.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "block_cache.h"

#include "lib/file/file_stats.h"
#include "lib/lockfree.h"
#include "lib/allocators/pool.h"


//-----------------------------------------------------------------------------

BlockId::BlockId()
	: m_atom_fn(0), m_blockIndex(~0u)
{
}

BlockId::BlockId(const char* atom_fn, off_t ofs)
{
	debug_assert(ofs <= (u64)blockSize * 0xFFFFFFFF);	// ensure value fits in m_blockIndex
	m_atom_fn = atom_fn;	// unique (by definition)
	m_blockIndex = (u32)(ofs / blockSize);
}

bool BlockId::operator==(const BlockId& rhs) const
{
	return m_atom_fn == rhs.m_atom_fn && m_blockIndex == rhs.m_blockIndex;
}

bool BlockId::operator!=(const BlockId& rhs) const
{
	return !operator==(rhs);
}


//-----------------------------------------------------------------------------

typedef LF_RefCountedMemRange Block;

class BlockManager
{
public:
	BlockManager(size_t numBlocks)
		: m_ids(numBlocks), m_blocks(numBlocks), m_oldestIndex(0)
	{
		for(size_t i = 0; i < m_blocks.size(); i++)
			m_blocks[i].mem = (void*)io_buf_Allocate(blockSize);
	}

	~BlockManager()
	{
		for(size_t i = 0; i < m_blocks.size(); i++)
			io_buf_Deallocate((IoBuf)m_blocks[i].mem, blockSize);
	}

	// (linear search is ok since we only expect to manage a few blocks)
	Block* Find(const BlockId& id)
	{
		for(size_t i = 0; i < m_ids.size(); i++)
		{
			if(m_ids[i] == id)
				return &m_blocks[i];
		}

		return 0;
	}

	Block* AcquireOldestAvailableBlock(BlockId id)
	{
		for(size_t i = 0; i < m_ids.size(); i++)
		{
			// (m_blocks are evicted in FIFO order.)
			Block& block = m_blocks[m_oldestIndex % m_blocks.size()];
			cpu_AtomicAdd(&m_oldestIndex, +1);

			if(block.refs.AcquireExclusiveAccess())
			{
				m_ids[i] = id;
				return &block;
			}

			// the oldest item is currently locked, so keep looking.
			//
			// to see when this can happen, consider IO depth = 4. let the
			// Block at m_blocks[oldest_block] contain data that an IO wants.
			// the 2nd and 3rd m_blocks are not in cache and happen to be taken
			// from near the end of m_blocks[]. attempting to issue block #4
			// fails because its buffer would want the first slot
			// (which is locked since its IO is still pending).
		}

		DEBUG_WARN_ERR(ERR::LIMIT);	// all m_blocks are locked
		return 0;
	}

	void InvalidateAll()
	{
		// note: don't check whether any references are held etc. because
		// this should only be called at the end of the (test) program.

		for(size_t i = 0; i < m_blocks.size(); i++)
			m_ids[i] = BlockId();
	}

private:
	std::vector<BlockId> m_ids;
	std::vector<Block> m_blocks;
	volatile intptr_t m_oldestIndex;
};


//-----------------------------------------------------------------------------

class BlockCache::Impl
{
public:
	Impl(size_t cacheSize)
		: m_blockManager(cacheSize / blockSize)
	{
	}

	IoBuf Reserve(BlockId id)
	{
		debug_assert(!m_blockManager.Find(id));	// must not already be extant

		Block* block = m_blockManager.AcquireOldestAvailableBlock(id);

#if CONFIG_READ_ONLY_CACHE
		mprotect(block->mem, block->size, PROT_WRITE|PROT_READ);
#endif

		return (IoBuf)block->mem;
	}

	void MarkComplete(BlockId id)
	{
		Block* block = m_blockManager.Find(id);
		debug_assert(block);	// (<id> cannot have been evicted because it is locked)

		block->refs.RelinquishExclusiveAccess();

#if CONFIG_READ_ONLY_CACHE
		mprotect(block.mem, block.size, PROT_READ);
#endif
	}

	IoBuf Retrieve(BlockId id)
	{
		Block* block = m_blockManager.Find(id);
		if(!block)		// not found
			return 0;

		if(!block->refs.AddReference())	// contents are not yet valid
			return 0;					// (this can happen due to multithreaded IOs)

		return (IoBuf)block->mem;
	}

	void Release(BlockId id)
	{
		Block* block = m_blockManager.Find(id);
		// (<id> ought not yet have been evicted because it is still referenced;
		// if not found, Release was called too often)
		debug_assert(block);

		block->refs.Release();
	}

	void InvalidateAll()
	{
		m_blockManager.InvalidateAll();
	}

private:
	BlockManager m_blockManager;
};


//-----------------------------------------------------------------------------

BlockCache::BlockCache(size_t cacheSize)
	: impl(new Impl(cacheSize))
{
}

IoBuf BlockCache::Reserve(BlockId id)
{
	return impl.get()->Reserve(id);
}

void BlockCache::MarkComplete(BlockId id)
{
	impl.get()->Reserve(id);
}

IoBuf BlockCache::Retrieve(BlockId id)
{
	return impl.get()->Retrieve(id);
}

void BlockCache::Release(BlockId id)
{
	impl.get()->Release(id);
}

void BlockCache::InvalidateAll()
{
	return impl.get()->InvalidateAll();
}
