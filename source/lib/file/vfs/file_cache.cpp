/**
 * =========================================================================
 * File        : file_cache.cpp
 * Project     : 0 A.D.
 * Description : cache of file contents (supports zero-copy IO)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "file_cache.h"

#include "lib/file/common/file_stats.h"
#include "lib/file/io/io_align.h"       // BLOCK_SIZE
#include "lib/cache_adt.h"              // Cache
#include "lib/bits.h"                   // round_up
#include "lib/allocators/allocators.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/allocators/headerless.h"
#include "lib/sysdep/os_cpu.h"	// os_cpu_PageSize


//-----------------------------------------------------------------------------
// allocator

/*
the biggest worry of a file cache is external fragmentation. there are two
basic ways to combat this:
1) 'defragment' periodically - move blocks around to increase
size of available 'holes'.
2) prevent fragmentation from occurring at all via
deliberate alloc/free policy.

file contents are returned directly to the user (zero-copy IO), so only
currently unreferenced blocks can be moved. it is believed that this would
severely hamper defragmentation; we therefore go with the latter approach.

the basic insight is: fragmentation occurs when a block is freed whose
neighbors are not free (thus preventing coalescing). this can be prevented by
allocating objects of similar lifetimes together. typical workloads
(uniform access frequency) already show such behavior: the Landlord cache
manager evicts files in an LRU manner, which matches the allocation policy.

references:
"The Memory Fragmentation Problem - Solved?" (Johnstone and Wilson)
"Dynamic Storage Allocation - A Survey and Critical Review" (Johnstone and Wilson)
*/

// shared_ptr<u8>s must own a reference to their allocator to ensure it's extant when
// they are freed. it is stored in the shared_ptr deleter.
class Allocator;
typedef shared_ptr<Allocator> PAllocator;

class FileCacheDeleter
{
public:
	FileCacheDeleter(size_t size, PAllocator allocator)
		: m_size(size), m_allocator(allocator)
	{
	}

	// (this uses Allocator and must come after its definition)
	void operator()(u8* mem) const;

private:
	size_t m_size;
	PAllocator m_allocator;
};


// adds statistics and AllocatorChecker to a HeaderlessAllocator
class Allocator
{
public:
	Allocator(size_t maxSize)
		: m_allocator(maxSize)
	{
	}

	shared_ptr<u8> Allocate(size_t size, PAllocator pthis)
	{
		const size_t alignedSize = round_up(size, BLOCK_SIZE);

		u8* mem = (u8*)m_allocator.Allocate(alignedSize);
		if(!mem)
			return DummySharedPtr<u8>(0);	// (prevent FileCacheDeleter from seeing a null pointer)

#ifndef NDEBUG
		m_checker.OnAllocate(mem, alignedSize);
#endif

		stats_buf_alloc(size, alignedSize);
		return shared_ptr<u8>(mem, FileCacheDeleter(size, pthis));
	}

	void Deallocate(u8* mem, size_t size)
	{
		const size_t alignedSize = round_up(size, BLOCK_SIZE);

		// (re)allow writes in case the buffer was made read-only. it would
		// be nice to unmap the buffer, but this is not possible because
		// HeaderlessAllocator needs to affix boundary tags.
		(void)mprotect(mem, size, PROT_READ|PROT_WRITE);

#ifndef NDEBUG
		m_checker.OnDeallocate(mem, alignedSize);
#endif
		m_allocator.Deallocate(mem, alignedSize);

		stats_buf_free();
	}

private:
	HeaderlessAllocator m_allocator;

#ifndef NDEBUG
	AllocatorChecker m_checker;
#endif
};


void FileCacheDeleter::operator()(u8* mem) const
{
	m_allocator->Deallocate(mem, m_size);
}


//-----------------------------------------------------------------------------
// FileCache::Impl
//-----------------------------------------------------------------------------

// since users are strongly encouraged to only load/process one file at a
// time, there won't be many active references to cache entries. we could
// take advantage of this with a separate extant list, but the cache's
// hash map should be fast enough and this way is less work than maintaining
// (possibly disjunct) cached and extant lists.

class FileCache::Impl
{
public:
	Impl(size_t maxSize)
		: m_allocator(new Allocator(maxSize))
	{
	}

	shared_ptr<u8> Reserve(size_t size)
	{
		// (should never happen because the VFS ensures size != 0.)
		debug_assert(size != 0);

		// (300 iterations have been observed when reserving several MB
		// of space in a full cache)
		for(;;)
		{
			{
				shared_ptr<u8> data = m_allocator->Allocate(size, m_allocator);
				if(data)
					return data;
			}

			// remove least valuable entry from cache (if users are holding
			// references, the contents won't actually be deallocated)
			{
				shared_ptr<u8> discardedData; size_t discardedSize;
				bool removed = m_cache.remove_least_valuable(&discardedData, &discardedSize);
				// only false if cache is empty, which can't be the case because
				// allocation failed.
				debug_assert(removed);
			}
		}
	}

	void Add(const VfsPath& pathname, shared_ptr<u8> data, size_t size, size_t cost)
	{
		// zero-copy cache => all users share the contents => must not
		// allow changes. this will be reverted when deallocating.
		(void)mprotect((void*)data.get(), size, PROT_READ);

		m_cache.add(pathname, data, size, cost);
	}

	bool Retrieve(const VfsPath& pathname, shared_ptr<u8>& data, size_t& size)
	{
		// (note: don't call stats_cache because we don't know the file size
		// in case of a cache miss; doing so is left to the caller.)
		stats_buf_ref();

		return m_cache.retrieve(pathname, data, &size);
	}

	void Remove(const VfsPath& pathname)
	{
		m_cache.remove(pathname);

		// note: we could check if someone is still holding a reference
		// to the contents, but that currently doesn't matter.
	}

private:
	typedef Cache< VfsPath, shared_ptr<u8> > Cache;
	Cache m_cache;

	PAllocator m_allocator;
};


//-----------------------------------------------------------------------------

FileCache::FileCache(size_t size)
	: impl(new Impl(size))
{
}

shared_ptr<u8> FileCache::Reserve(size_t size)
{
	return impl->Reserve(size);
}

void FileCache::Add(const VfsPath& pathname, shared_ptr<u8> data, size_t size, size_t cost)
{
	impl->Add(pathname, data, size, cost);
}

void FileCache::Remove(const VfsPath& pathname)
{
	impl->Remove(pathname);
}

bool FileCache::Retrieve(const VfsPath& pathname, shared_ptr<u8>& data, size_t& size)
{
	return impl->Retrieve(pathname, data, size);
}
