/**
 * =========================================================================
 * File        : block_cache.h
 * Project     : 0 A.D.
 * Description : cache for aligned I/O blocks.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_BLOCK_CACHE
#define INCLUDED_BLOCK_CACHE

#include "lib/file/path.h"

/**
 * ID that uniquely identifies a block within a file
 **/
class BlockId
{
public:
	BlockId();
	BlockId(const Path& pathname, off_t ofs);
	bool operator==(const BlockId& rhs) const;
	bool operator!=(const BlockId& rhs) const;

private:
	u64 m_id;
};


/**
 * cache of (aligned) file blocks with support for zero-copy IO.
 * absorbs the overhead of rounding up archive IOs to the nearest block
 * boundaries by keeping the last few blocks in memory.
 *
 * the interface is somewhat similar to FileCache; see the note there.
 *
 * not thread-safe (each thread is intended to have its own cache).
 **/
class BlockCache
{
public:
	/**
	 * @param numBlocks (the default value is enough to support temp buffers
	 * and absorb the cost of unaligned reads from archives.)
	 **/
	BlockCache(size_t numBlocks = 16);

	/**
	 * Add a block to the cache.
	 *
	 * @param id key that will be used to Retrieve the block.
	 *
	 * call this when the block's IO has completed; its data will
	 * satisfy subsequent Retrieve calls for the same id.
	 * if CONFIG2_CACHE_READ_ONLY, the memory is made read-only.
	 **/
	void Add(BlockId id, const shared_ptr<u8>& buf);

	/**
	 * Attempt to retrieve a block's contents.
	 *
	 * @return whether the block is in cache.
	 *
	 * if successful, a shared pointer to the contents is returned.
	 * they remain valid until all references are removed and the block
	 * is evicted.
	 **/
	bool Retrieve(BlockId id, shared_ptr<u8>& buf);

	/**
	 * Invalidate the contents of the cache.
	 *
	 * this effectively discards the contents of existing blocks
	 * (more specifically: prevents them from satisfying Retrieve calls
	 * until a subsequent Add with the same id).
	 *
	 * useful for self-tests: multiple independent IO tests run in the same
	 * process and must not influence each other via the cache.
	 **/
	void InvalidateAll();

private:
	class Impl;
	shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_BLOCK_CACHE
