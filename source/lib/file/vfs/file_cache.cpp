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

#include "path.h"
#include "file_stats.h"
#include "archive/trace.h"
#include "lib/cache_adt.h"				// Cache
#include "lib/bits.h"					// round_up
#include "lib/allocators/allocators.h"
#include "lib/allocators/headerless.h"
#include "lib/allocators/mem_util.h"	// mem_PageSize


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

class Allocator;

class Deleter
{
public:
	Deleter(size_t size, Allocator* allocator, const char* owner)
		: m_size(size), m_allocator(allocator), m_owner(owner)
	{
	}

	// (this must come after Allocator because it calls Deallocate())
	void operator()(const u8* data) const;

private:
	size_t m_size;
	Allocator* m_allocator;
	const char* m_owner;
};

// >= sys_max_sector_size or else waio will have to realign.
// chosen as exactly 1 page: this allows write-protecting file buffers
// without worrying about their (non-page-aligned) borders.
// internal fragmentation is considerable but acceptable.
static const size_t alignment = mem_PageSize();

class Allocator
{
public:
	Allocator(size_t maxSize)
		: m_allocator(maxSize)
	{
	}

	FileCacheData Allocate(size_t size, const char* owner)
	{
		const size_t alignedSize = round_up(size, alignment);
		stats_buf_alloc(size, alignedSize);

		const u8* data = (const u8*)m_allocator.Allocate(alignedSize);
#ifndef NDEBUG
		m_checker.notify_alloc((void*)data, alignedSize);
#endif

		return FileCacheData(data, Deleter(size, this, owner));
	}

	void Deallocate(const u8* data, size_t size, const char* owner)
	{
		void* const p = (void*)data;
		const size_t alignedSize = round_up(size, alignment);

		// (re)allow writes. it would be nice to un-map the buffer, but this is
		// not possible because HeaderlessAllocator needs to affix boundary tags.
		(void)mprotect(p, size, PROT_READ|PROT_WRITE);

#ifndef NDEBUG
		m_checker.notify_free(p, alignedSize);
#endif
		m_allocator.Deallocate(p, alignedSize);

		stats_buf_free();
		trace_notify_free(owner);
	}

private:
	HeaderlessAllocator m_allocator;

#ifndef NDEBUG
	AllocatorChecker m_checker;
#endif
};


void Deleter::operator()(const u8* data) const
{
	m_allocator->Deallocate(data, m_size, m_owner);
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
	Impl(size_t size)
		: m_allocator(size)
	{
	}

	FileCacheData Reserve(const char* vfsPathname, size_t size)
	{
		// (this probably indicates a bug; caching 0-length files would
		// have no benefit, anyway)
		debug_assert(size != 0);

		// (300 iterations have been observed when reserving several MB
		// of space in a full cache)
		for(;;)
		{
			FileCacheData data = m_allocator.Allocate(size, vfsPathname);
			if(data.get())
				return data;

			// remove least valuable entry from cache (if users are holding
			// references, the contents won't actually be deallocated)
			FileCacheData discardedData; size_t discardedSize;
			bool removed = m_cache.remove_least_valuable(&discardedData, &discardedSize);
			// only false if cache is empty, which can't be the case because
			// allocation failed.
			debug_assert(removed);
		}
	}

	void Add(const char* vfsPathname, FileCacheData data, size_t size, uint cost)
	{
		// zero-copy cache => all users share the contents => must not
		// allow changes. this will be reverted when deallocating.
		(void)mprotect((void*)data.get(), size, PROT_READ);

		m_cache.add(vfsPathname, data, size, cost);
	}

	bool Retrieve(const char* vfsPathname, FileCacheData& data, size_t& size)
	{
		// (note: don't call stats_cache because we don't know the file size
		// in case of a cache miss; doing so is left to the caller.)
		stats_buf_ref();

		return m_cache.retrieve(vfsPathname, data, &size);
	}

	void Remove(const char* vfsPathname)
	{
		m_cache.remove(vfsPathname);

		// note: we could check if someone is still holding a reference
		// to the contents, but that currently doesn't matter.
	}

private:
	// HACK: due to vfsPathname, we are assured that strings are equal iff their
	// addresses match. however, Cache's STL (hash_)map stupidly assumes that
	// const char* keys are "strings". to avoid this behavior, we specify the
	// key as const void*.
	static Cache<const void*, FileCacheData> m_cache;

	Allocator m_allocator;
};


//-----------------------------------------------------------------------------

FileCache::FileCache(size_t size)
	: impl(new Impl(size))
{
}

FileCacheData FileCache::Reserve(const char* vfsPathname, size_t size)
{
	return impl.get()->Reserve(vfsPathname, size);
}

void FileCache::Add(const char* vfsPathname, FileCacheData data, size_t size, uint cost)
{
	impl.get()->Add(vfsPathname, data, size, cost);
}

void FileCache::Remove(const char* vfsPathname)
{
	impl.get()->Remove(vfsPathname);
}

bool FileCache::Retrieve(const char* vfsPathname, FileCacheData& data, size_t& size)
{
	return impl.get()->Retrieve(vfsPathname, data, size);
}
