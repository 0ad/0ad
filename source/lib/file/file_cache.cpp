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

#include <map>

#include "path.h"
#include "file_stats.h"
#include "archive/trace.h"
#include "lib/cache_adt.h"				// Cache
#include "lib/lockfree.h"
#include "lib/bits.h"					// round_up
#include "lib/allocators/allocators.h"
#include "lib/allocators/headerless.h"
#include "lib/allocators/mem_util.h"	// mem_PageSize


// >= sys_max_sector_size or else waio will have to realign.
// chosen as exactly 1 page: this allows write-protecting file buffers
// without worrying about their (non-page-aligned) borders.
// internal fragmentation is considerable but acceptable.
static const size_t alignment = mem_PageSize();


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

class Allocator
{
public:
	Allocator(size_t maxSize)
		: m_allocator(maxSize)
	{
	}

	IoBuf Allocate(size_t size)
	{
		const size_t alignedSize = round_up(size, alignment);
		stats_buf_alloc(size, alignedSize);

		void* p = m_allocator.Allocate(alignedSize);
#ifndef NDEBUG
		m_checker.notify_alloc(p, alignedSize);
#endif

		return (IoBuf)p;
	}

	void Deallocate(IoBuf buf, size_t size)
	{
		void* const p = (void*)buf;

		// (re)allow writes. it would be nice to un-map the buffer, but this is
		// not possible because HeaderlessAllocator needs to affix boundary tags.
		(void)mprotect(p, size, PROT_READ|PROT_WRITE);

		const size_t alignedSize = round_up(size, alignment);
#ifndef NDEBUG
		m_checker.notify_free(p, alignedSize);
#endif
		m_allocator.Deallocate(p, alignedSize);

		stats_buf_free();
	}

private:
	HeaderlessAllocator m_allocator;

#ifndef NDEBUG
	static AllocatorChecker m_checker;
#endif
};


//-----------------------------------------------------------------------------

typedef LF_RefCountedMemRange FileContents;

/**
 * manages the active FileContents referenced by users.
 * ("active" means between Reserve() and the final Release())
 **/
class ActiveList
{
public:
	~ActiveList()
	{
		// display leaks
		debug_printf("file_cache leaks:\n");
		for(MapIt it = m_map.begin(); it != m_map.end(); ++it)
		{
			const char* atom_fn = it->first;
			FileContents& fc = it->second;
			debug_printf("  %s (0x%P 0x%08x)\n", atom_fn, fc.mem, fc.size);
		}
		debug_printf("--------\n");
	}

	void Add(const char* atom_fn, FileContents& fc)
	{
		const PairIB ret = m_map.insert(std::make_pair(atom_fn, fc));
		debug_assert(ret.second);	// complain if already existed
	}

	void Remove(const char* atom_fn)
	{
		const size_t numRemoved = m_map.erase(atom_fn);
		debug_assert(numRemoved == 1);
	}

	FileContents* Find(const char* atom_fn)
	{
		MapIt it = m_map.find(atom_fn);
		if(it == m_map.end())
			return 0;	// not an error
		return &it->second;
	}

	// (called by FileCache::Impl::AllocateCacheSpace; we can't pass
	// atom_fn because Cache only knows about buf and size.)
	bool Contains(IoBuf buf) const
	{
		for(MapCIt it = m_map.begin(); it != m_map.end(); ++it)
		{
			const FileContents& fc = it->second;
			if(fc.mem == buf)
				return true;
		}

		return false;
	}

private:
	typedef std::map<const char*, FileContents> Map;
	typedef Map::iterator MapIt;
	typedef Map::const_iterator MapCIt;
	typedef std::pair<MapIt, bool> PairIB;
	Map m_map;
};


//-----------------------------------------------------------------------------
// FileCache::Impl
//-----------------------------------------------------------------------------

// the organization of this cache is somewhat counterintuitive. one might
// expect a simple mapping of filename to FileContents. however, since users
// are strongly encouraged to only load/process one file at a time, there
// will only be a few active references. with the cache holding many more
// entries, looking up files there is more expensive than consulting a
// separate list of active FileContents.
// this list (the "manager") and the cache contents are not necessarily
// related; no inclusion relation need hold. the only requirement is that
// each consult the other on ownership issues. if the cache decides a file
// should be evicted while references to it are active, or users release a
// reference to FileContents that the cache wants to keep, the memory must
// not actually be freed. (it is then logically 'owned' by the other)

class FileCache::Impl
{
public:
	Impl(size_t size)
		: m_allocator(size)
	{
	}

	IoBuf Reserve(const char* atom_fn, size_t size)
	{
		// (this probably indicates a bug; caching 0-length files would
		// have no benefit, anyway)
		debug_assert(size != 0);

		IoBuf buf = AllocateCacheSpace(size);

		FileContents fc;
		fc.refs.AcquireExclusiveAccess();
		fc.mem = (void*)buf;
		fc.size = size;
		m_activeList.Add(atom_fn, fc);

		return buf;
	}

	void MarkComplete(const char* atom_fn, uint cost)
	{
		FileContents* fc = m_activeList.Find(atom_fn);
		debug_assert(fc);

		fc->refs.RelinquishExclusiveAccess();

		// zero-copy cache => all users share the contents => must not
		// allow changes. this will be reverted when the buffer is freed.
		(void)mprotect(fc->mem, fc->size, PROT_READ);

		m_cache.add(atom_fn, (IoBuf)fc->mem, fc->size, cost);
	}

	IoBuf Retrieve(const char* atom_fn, size_t& size)
	{
		IoBuf buf;
		if(!m_cache.retrieve(atom_fn, buf, &size))
			return 0;

		FileContents* pfc = m_activeList.Find(atom_fn);
		// was already active; add a reference.
		if(pfc)
			pfc->refs.AddReference();
		// in cache, but no active references; add to list.
		else
		{
			FileContents fc;
			fc.refs.AddReference();
			fc.mem = (void*)buf;
			fc.size = size;
			m_activeList.Add(atom_fn, fc);
		}

		stats_buf_ref();
		return buf;
	}

	void Release(const char* atom_fn)
	{
		FileContents* fc = m_activeList.Find(atom_fn);
		debug_assert(fc);

		fc->refs.Release();
		if(fc->refs.ReferenceCount() == 0)
		{
			trace_notify_free(atom_fn);

			if(!IsInCache(atom_fn))
				m_allocator.Deallocate((IoBuf)fc->mem, fc->size);

			m_activeList.Remove(atom_fn);
		}
	}

	LibError Invalidate(const char* atom_fn)
	{
		// remove from cache
		IoBuf cachedBuf; size_t cachedSize;
		if(m_cache.peek(atom_fn, cachedBuf, &cachedSize))
		{
			m_cache.remove(atom_fn);
			// note: we ensure cachedBuf is not active below.
			m_allocator.Deallocate(cachedBuf, cachedSize);
		}

		// this could happen if a hotload notification comes while someone
		// is holding a reference to the file contents. atom_fn has been
		// removed from the cache, so subsequent Retrieve() calls will not
		// return old data. however, (re)loading the file would fail because
		// Reserve() ensures there's not already an extant buffer.
		// the correct way to handle this is to delay or cancel the reload,
		// so we notify our caller accordingly.
		if(IsActive(cachedBuf))
			WARN_RETURN(ERR::AGAIN);	// if this actually happens, remove the warning.

		return INFO::OK;
	}

private:
	bool IsActive(IoBuf buf) const
	{
		return m_activeList.Contains(buf);
	}

	bool IsInCache(const char* atom_fn) const
	{
		IoBuf cachedBuf; size_t cachedSize;	// unused
		return m_cache.peek(atom_fn, cachedBuf, &cachedSize);
	}

	IoBuf AllocateCacheSpace(size_t size)
	{
		uint attempts = 0;
		for(;;)
		{
			IoBuf buf = m_allocator.Allocate(size);
			if(buf)
				return buf;

			// remove least valuable entry from cache
			IoBuf discardedBuf; size_t discardedSize;
			bool removed = m_cache.remove_least_valuable(&discardedBuf, &discardedSize);
			// only false if cache is empty, which can't be the case because
			// allocation failed.
			debug_assert(removed);

			// someone is holding a reference; we must not free the
			// underlying memory, nor count this iteration.
			if(IsActive(discardedBuf))
				continue;

			m_allocator.Deallocate(discardedBuf, discardedSize);

			// note: this may seem hefty, but 300 is known to be reached.
			// (after building an archive, the file cache will be full;
			// attempting to allocate a few MB can take a while if only
			// small scattered blocks are freed.)
			debug_assert(++attempts < 500);	// otherwise: failed to make room in cache?!
		}
	}

	ActiveList m_activeList;

	// HACK: due to atom_fn, we are assured that strings are equal iff their
	// addresses match. however, Cache's STL (hash_)map stupidly assumes that
	// const char* keys are "strings". to avoid this behavior, we specify the
	// key as const void*.
	static Cache<const void*, IoBuf> m_cache;

	Allocator m_allocator;
};


//-----------------------------------------------------------------------------

FileCache::FileCache(size_t size)
	: impl(new Impl(size))
{
}

IoBuf FileCache::Reserve(const char* atom_fn, size_t size)
{
	return impl.get()->Reserve(atom_fn, size);
}

void FileCache::MarkComplete(const char* atom_fn, uint cost)
{
	impl.get()->MarkComplete(atom_fn, cost);
}

IoBuf FileCache::Retrieve(const char* atom_fn, size_t& size)
{
	return impl.get()->Retrieve(atom_fn, size);
}

void FileCache::Release(const char* atom_fn)
{
	impl.get()->Release(atom_fn);
}

LibError FileCache::Invalidate(const char* atom_fn)
{
	return impl.get()->Invalidate(atom_fn);
}
