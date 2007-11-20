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

#include "io_buf.h"

/**
 * block := power-of-two sized chunk of a file.
 * all transfers are expanded to naturally aligned, whole blocks
 * (this makes caching parts of files feasible; it is also much faster
 * for some aio implementations, e.g. wposix).
 *
 * measurements show this value to yield best read throughput.
 **/
static const size_t BLOCK_SIZE = 32*KiB;


/**
 * ID that uniquely identifies a block within a file
 **/
class BlockId
{
public:
	BlockId();
	BlockId(const char* atom_fn, off_t ofs);
	bool operator==(const BlockId& rhs) const;
	bool operator!=(const BlockId& rhs) const;

private:
	const char* m_atom_fn;
	u32 m_blockIndex;
};


/**
 * cache of (aligned) file blocks with support for zero-copy IO.
 * absorbs the overhead of rounding up archive IOs to the nearest block
 * boundaries by keeping the last few blocks in memory.
 *
 * the interface is quite similar to FileCache; see the note there.
 **/
class BlockCache
{
public:
	/**
	 * @param cacheSize total size [bytes] that the cache is to manage.
	 * the default value is enough to support temp buffers and
	 * absorb the cost of unaligned reads from a few archives.
	 **/
	BlockCache(size_t cacheSize = 16 * BLOCK_SIZE);

	/**
	 * Reserve a block for use as an IO buffer.
	 *
	 * @return suitably aligned memory; never fails.
	 *
	 * no further operations with the same id are allowed to succeed
	 * until MarkComplete has been called.
	 **/
	IoBuf Reserve(BlockId id);

	/**
	 * Indicate that IO into the block has completed.
	 *
	 * this allows the cache to satisfy subsequent Retrieve() calls by
	 * returning this block; if CONFIG_READ_ONLY_CACHE, the block is
	 * made read-only. if need be and no references are currently attached
	 * to it, the memory can also be commandeered by Reserve().
	 **/
	void MarkComplete(BlockId id);

	/**
	 * Attempt to retrieve a block the file cache.
	 *
	 * @return false if not in cache or its IO is still pending,
	 * otherwise true.
	 *
	 * if successful, a reference is added to the block and its
	 * buffer is returned.
	 **/
	bool Retrieve(BlockId id, IoBuf& buf);

	/**
	 * Indicate the block contents are no longer needed.
	 *
	 * this decreases the reference count; the memory can only be reused
	 * if it reaches 0. the block remains in cache until evicted by a
	 * subsequent Reserve() call.
	 *
	 * note: fails (raises a warning) if called for a buffer that is
	 * currently between Reserve and MarkComplete operations.
	 **/
	void Release(BlockId id);

	/**
	 * Invalidate the contents of the cache.
	 *
	 * this effectively discards the contents of existing blocks
	 * (more specifically: prevents them from satisfying Retrieve() calls
	 * until a subsequent Reserve/MarkComplete of that block).
	 *
	 * useful for self-tests: multiple independent IO tests run in the same
	 * process and must not influence each other via the cache.
	 **/
	void InvalidateAll();

private:
	class Impl;
	boost::shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_BLOCK_CACHE
