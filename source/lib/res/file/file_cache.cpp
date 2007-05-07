/**
 * =========================================================================
 * File        : file_cache.cpp
 * Project     : 0 A.D.
 * Description : cache for entire files and I/O blocks. also allocates
 *             : file buffers, allowing zero-copy I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "file_cache.h"

#include <map>

#include "lib/posix/posix_mman.h"
#include "lib/allocators.h"
#include "lib/byte_order.h"
#include "lib/cache_adt.h"
#include "file_internal.h"

//-----------------------------------------------------------------------------

// block cache: intended to cache raw compressed data, since files aren't aligned
// in the archive; alignment code would force a read of the whole block,
// which would be a slowdown unless we keep them in memory.
//
// keep out of async code (although extra work for sync: must not issue/wait
// if was cached) to simplify things. disadvantage: problems if same block
// is issued twice, before the first call completes (via wait_io).
// that won't happen though unless we have threaded file_ios =>
// rare enough not to worry about performance.
//
// since sync code allocates the (temp) buffer, it's guaranteed
// to remain valid.
//

class BlockMgr
{
	static const size_t MAX_BLOCKS = 32;
	enum BlockStatus
	{
		BS_PENDING,
		BS_COMPLETE,
		BS_INVALID
	};
	struct Block
	{
		BlockId id;
		// initialized in BlockMgr ctor and remains valid
		void* mem;
		BlockStatus status;
		int refs;

		Block()
			: id(block_cache_make_id(0, 0)), status(BS_INVALID), refs(0) {}
	};
	// access pattern is usually ring buffer, but in rare cases we
	// need to skip over locked items, even though they are the oldest.
	Block blocks[MAX_BLOCKS];
	uint oldest_block;

	// use Pool to allocate mem for all blocks because it guarantees
	// page alignment (required for IO) and obviates manually aligning.
	Pool pool;

public:
	BlockMgr()
		: blocks(), oldest_block(0)
	{
		(void)pool_create(&pool, MAX_BLOCKS*FILE_BLOCK_SIZE, FILE_BLOCK_SIZE);
		for(Block* b = blocks; b < blocks+MAX_BLOCKS; b++)
		{
			b->mem = pool_alloc(&pool, 0);
			debug_assert(b->mem);	// shouldn't ever fail
		}
	}

	void shutdown()
	{
		(void)pool_destroy(&pool);
	}

	void* alloc(BlockId id)
	{
		Block* b;
		for(b = blocks; b < blocks+MAX_BLOCKS; b++)
		{
			if(block_eq(b->id, id))
				debug_warn("allocating block that is already in list");
		}

		for(size_t i = 0; i < MAX_BLOCKS; i++)
		{
			b = &blocks[oldest_block];
			oldest_block = (oldest_block+1)%MAX_BLOCKS;

			// normal case: oldest item can be reused
			if(b->status != BS_PENDING && b->refs == 0)
				goto have_block;

			// wacky special case: oldest item is currently locked.
			// skip it and reuse the next.
			//
			// to see when this can happen, consider IO depth = 4.
			// let the Block at blocks[oldest_block] contain data that
			// an IO wants. the 2nd and 3rd blocks are not in cache and
			// happen to be taken from near the end of blocks[].
			// attempting to issue block #4 fails because its buffer would
			// want the first slot (which is locked since the its IO
			// is still pending).
			if(b->status == BS_COMPLETE && b->refs > 0)
				continue;

			debug_warn("status and/or refs have unexpected values");
		}

		debug_warn("all blocks are locked");
		return 0;
have_block:

		b->id = id;
		b->status = BS_PENDING;
		return b->mem;
	}

	void mark_completed(BlockId id)
	{
		for(Block* b = blocks; b < blocks+MAX_BLOCKS; b++)
		{
			if(block_eq(b->id, id))
			{
				debug_assert(b->status == BS_PENDING);
				b->status = BS_COMPLETE;
				return;
			}
		}
		debug_warn("mark_completed: block not found, but ought still to be in cache");
	}

	void* find(BlockId id)
	{
		// linear search is ok, since we only keep a few blocks.
		for(Block* b = blocks; b < blocks+MAX_BLOCKS; b++)
		{
			if(block_eq(b->id, id))
			{
				 if(b->status == BS_COMPLETE)
				 {
					 debug_assert(b->refs >= 0);
					 b->refs++;
					 return b->mem;
				 }

				 debug_warn("block referenced while still in progress");
				 return 0;
			}
		}
		return 0;	// not found
	}

	void release(BlockId id)
	{
		for(Block* b = blocks; b < blocks+MAX_BLOCKS; b++)
		{
			if(block_eq(b->id, id))
			{
				b->refs--;
				debug_assert(b->refs >= 0);
				return;
			}
		}
		debug_warn("release: block not found, but ought still to be in cache");
	}

	void invalidate(const char* atom_fn)
	{
		for(Block* b = blocks; b < blocks+MAX_BLOCKS; b++)
		{
			if(b->id.atom_fn == atom_fn)
			{
				if(b->refs)
					debug_warn("invalidating block that is currently in-use");
				b->status = BS_INVALID;
			}
		}
	}
};
static BlockMgr block_mgr;


bool block_eq(BlockId b1, BlockId b2)
{
	return b1.atom_fn == b2.atom_fn && b1.block_num == b2.block_num;
}

// create an id for use with the cache that uniquely identifies
// the block from the file <atom_fn> starting at <ofs>.
BlockId block_cache_make_id(const char* atom_fn, const off_t ofs)
{
	// <atom_fn> is guaranteed to be unique (see file_make_unique_fn_copy).
	// block_num should always fit in 32 bits (assuming maximum file size
	// = 2^32 * FILE_BLOCK_SIZE ~= 2^48 -- plenty). we don't bother
	// checking this.
	const u32 block_num = (u32)(ofs / FILE_BLOCK_SIZE);
	BlockId id = { atom_fn, block_num };
	return id;
}

void* block_cache_alloc(BlockId id)
{
	return block_mgr.alloc(id);
}

void block_cache_mark_completed(BlockId id)
{
	block_mgr.mark_completed(id);
}

void* block_cache_find(BlockId id)
{
	void* ret = block_mgr.find(id);
	stats_block_cache(ret? CR_HIT : CR_MISS);
	return ret;
}

void block_cache_release(BlockId id)
{
	return block_mgr.release(id);
}


//-----------------------------------------------------------------------------

// >= file_sector_size or else waio will have to realign.
// chosen as exactly 1 page: this allows write-protecting file buffers
// without worrying about their (non-page-aligned) borders.
// internal fragmentation is considerable but acceptable.
static const size_t BUF_ALIGN = 4*KiB;

/*
CacheAllocator

the biggest worry of a file cache is fragmentation. there are 2
basic approaches to combat this:
1) 'defragment' periodically - move blocks around to increase
   size of available 'holes'.
2) prevent fragmentation from occurring at all via
   deliberate alloc/free policy.

file_io returns cache blocks directly to the user (zero-copy IO),
so only currently unreferenced blocks can be moved (while holding a
lock, to boot). it is believed that this would severely hamper
defragmentation; we therefore go with the latter approach.

basic insight is: fragmentation occurs when a block is freed whose
neighbors are not free (thus preventing coalescing). this can be
prevented by allocating objects of similar lifetimes together.
typical workloads (uniform access frequency) already show such behavior:
the Landlord cache manager evicts files in an LRU manner, which matches
the allocation policy.

references:
"The Memory Fragmentation Problem - Solved?" (Johnstone and Wilson)
"Dynamic Storage Allocation - A Survey and Critical Review" (Johnstone and Wilson)

policy:
- allocation: use all available mem first, then look at freelist
- freelist: good fit, address-ordered, always split blocks
- free: immediately coalesce
mechanism:
- coalesce: boundary tags in freed memory with magic value
- freelist: 2**n segregated doubly-linked, address-ordered
*/
static const size_t MAX_CACHE_SIZE = 96*MiB;

class CacheAllocator
{
public:
	CacheAllocator()
		: bitmap(0), freelists()
	{
		// (safe to call this from ctor as of 2006-02-02)
		(void)pool_create(&pool, MAX_CACHE_SIZE, 0);
	}

	void shutdown()
	{
		(void)pool_destroy(&pool);
	}

	void* alloc(size_t size)
	{
		// determine actual size to allocate
		// .. better not be more than MAX_CACHE_SIZE - file_buf_alloc will
		//    fail because no amount of freeing up existing allocations
		//    would make enough room. therefore, check for this here
		//    (should never happen).
		debug_assert(size < MAX_CACHE_SIZE);
		// .. safely handle 0 byte allocations. according to C/C++ tradition,
		//    we allocate a unique address, which ends up wasting 1 page.
		if(!size)
			size = 1;
		// .. each allocation must be aligned to BUF_ALIGN, so
		//    we round up all sizes to that.
		const size_t size_pa = round_up(size, BUF_ALIGN);
		const uint size_class = size_class_of(size_pa);

		void* p;

		// try to reuse a freed entry
		p = alloc_from_class(size_class, size_pa);
		if(p)
			goto success;

		// grab more space from pool
		p = pool_alloc(&pool, size_pa);
		if(p)
			goto success;

		// last resort: split a larger element
		p = alloc_from_larger_class(size_class, size_pa);
		if(p)
			goto success;

		// failed - can no longer expand and nothing big enough was
		// found in freelists.
		// file cache will decide which elements are least valuable,
		// free() those and call us again.
		return 0;

success:
#ifndef NDEBUG
		alloc_checker.notify_alloc(p, size);
#endif
		stats_notify_alloc(size_pa);
		return p;
	}

	// rationale: don't call this "free" because that would run afoul of the
	// memory tracker's redirection macro and require #include "lib/nommgr.h".
	void dealloc(u8* p, size_t size)
	{
#ifndef NDEBUG
		alloc_checker.notify_free(p, size);
#endif

		const size_t size_pa = round_up(size, BUF_ALIGN);
		// make sure entire (aligned!) range is within pool.
		if(!pool_contains(&pool, p) || !pool_contains(&pool, p+size_pa-1))
		{
			debug_warn("invalid pointer");
			return;
		}

		// (re)allow writes
		//
		// note: unfortunately we cannot unmap this buffer's memory
		// (to make sure it is not used) because we write a header/footer
		// into it to support coalescing.
		(void)mprotect(p, size_pa, PROT_READ|PROT_WRITE);

		coalesce_and_free(p, size_pa);

		stats_notify_free(size_pa);
	}

	// make given range read-only via MMU.
	// write access is restored when buffer is freed.
	//
	// p and size are the exact (non-padded) values as in dealloc.
	void make_read_only(u8* p, size_t size)
	{
		// bail to avoid mprotect failing
		if(!size)
			return;

		const size_t size_pa = round_up(size, BUF_ALIGN);
		(void)mprotect(p, size_pa, PROT_READ);
	}

	// free all allocations and reset state to how it was just after
	// (the first and only) init() call.
	void reset()
	{
#ifndef NDEBUG
		alloc_checker.notify_clear();
#endif

		pool_free_all(&pool);
		bitmap = 0;
		memset(freelists, 0, sizeof(freelists));
		stats_reset();
	}

private:
#ifndef NDEBUG
	AllocatorChecker alloc_checker;
#endif

	Pool pool;

	//-------------------------------------------------------------------------
	// boundary tags for coalescing
	static const u32 HEADER_ID = FOURCC('C','M','A','H');
	static const u32 FOOTER_ID = FOURCC('C','M','A','F');
	static const u32 MAGIC = FOURCC('\xFF','\x55','\xAA','\x01');
	struct Header
	{
		Header* prev;
		Header* next;
		size_t size_pa;
		u32 id;
		u32 magic;
	};
	// we could use struct Header for Footer as well, but keeping them
	// separate and different can avoid coding errors (e.g. mustn't pass a
	// Footer to freelist_remove!)
	struct Footer
	{
		// note: deliberately reordered fields for safety
		u32 magic;
		u32 id;
		size_t size_pa;
	};
	// must be enough room to stash Header+Footer within the freed allocation.
	cassert(BUF_ALIGN >= sizeof(Header)+sizeof(Footer));

	// expected_id identifies the tag type (either HEADER_ID or
	// FOOTER_ID). returns whether the given id, magic and size_pa
	// values are consistent with such a tag.
	//
	// note: these magic values are all that differentiates tags from
	// user data. this isn't 100% reliable, but we can't insert extra
	// boundary tags because the memory must remain aligned.
	bool is_valid_tag(u32 expected_id, u32 id, u32 magic, size_t size_pa) const
	{
		if(id != expected_id || magic != MAGIC)
			return false;
		debug_assert(size_pa % BUF_ALIGN == 0);
		debug_assert(size_pa <= MAX_CACHE_SIZE);
		return true;
	}

	// add p to freelist; if its neighbor(s) are free, merges them all into
	// one big region and frees that.
	// notes:
	// - correctly deals with p lying at start/end of pool.
	// - p and size_pa are trusted: [p, p+size_pa) lies within the pool.
	void coalesce_and_free(u8* p, size_t size_pa)
	{
		// CAVEAT: Header and Footer are wiped out by freelist_remove -
		// must use them before that.

		// expand (p, size_pa) to include previous allocation if it's free.
		// (unless p is at start of pool region)
		if(p != pool.da.base)
		{
			const Footer* footer = (const Footer*)(p-sizeof(Footer));
			if(is_valid_tag(FOOTER_ID, footer->id, footer->magic, footer->size_pa))
			{
				p       -= footer->size_pa;
				size_pa += footer->size_pa;
				Header* header = (Header*)p;
				freelist_remove(header);
			}
		}

		// expand size_pa to include following memory if it was allocated
		// and is currently free.
		// (unless it starts beyond end of currently committed region)
		Header* header = (Header*)(p+size_pa);
		if((u8*)header < pool.da.base+pool.da.cur_size)
		{
			if(is_valid_tag(HEADER_ID, header->id, header->magic, header->size_pa))
			{
				size_pa += header->size_pa;
				freelist_remove(header);
			}
		}

		freelist_add(p, size_pa);
	}

	//-------------------------------------------------------------------------
	// freelist

	// segregated, i.e. one list per size class.
	// note: we store Header nodes instead of just a pointer to head of
	// list - this wastes a bit of mem but greatly simplifies list insertion.
	Header freelists[sizeof(uintptr_t)*CHAR_BIT];

	// bit i set iff size class i's freelist is not empty.
	// in conjunction with ls1, this allows finding a non-empty list in O(1).
	uintptr_t bitmap;

	// "size class" i (>= 0) contains allocations of size (2**(i-1), 2**i]
	// except for i=0, which corresponds to size=1.
	static uint size_class_of(size_t size_pa)
	{
		return log2((uint)size_pa);
	}

	// value of LSB 1-bit.
	static uint ls1(uint x)
	{
		return (x & -(int)x);
	}

	void freelist_add(u8* p, size_t size_pa)
	{
		debug_assert((uintptr_t)p % BUF_ALIGN == 0);
		debug_assert(size_pa % BUF_ALIGN == 0);
		const uint size_class = size_class_of(size_pa);

		// write header and footer into the freed mem
		// (its prev and next link fields will be set below)
		Header* header = (Header*)p;
		header->id = HEADER_ID;
		header->magic = MAGIC;
		header->size_pa = size_pa;
		Footer* footer = (Footer*)(p+size_pa-sizeof(Footer));
		footer->id = FOOTER_ID;
		footer->magic = MAGIC;
		footer->size_pa = size_pa;

		Header* prev = &freelists[size_class];
		// find node after which to insert (address ordered freelist)
		while(prev->next && header <= prev->next)
			prev = prev->next;

		header->next = prev->next;
		header->prev = prev;
		if(prev->next)
			prev->next->prev = header;
		prev->next = header;

        bitmap |= BIT(size_class);
	}

	void freelist_remove(Header* header)
	{
		debug_assert((uintptr_t)header % BUF_ALIGN == 0);

		Footer* footer = (Footer*)((u8*)header+header->size_pa-sizeof(Footer));
		debug_assert(is_valid_tag(HEADER_ID, header->id, header->magic, header->size_pa));
		debug_assert(is_valid_tag(FOOTER_ID, footer->id, footer->magic, footer->size_pa));
		debug_assert(header->size_pa == footer->size_pa);
		const uint size_class = size_class_of(header->size_pa);

		header->prev->next = header->next;
		if(header->next)
			header->next->prev = header->prev;

		// if freelist is now empty, clear bit in bitmap.
		if(!freelists[size_class].next)
			bitmap &= ~BIT(size_class);

		// wipe out header and footer to prevent accidental reuse
		memset(header, 0xEE, sizeof(Header));
		memset(footer, 0xEE, sizeof(Footer));
	}

	// returns 0 if nothing big enough is in size_class's freelist.
	void* alloc_from_class(uint size_class, size_t size_pa)
	{
		// return first suitable entry in (address-ordered) list
		for(Header* cur = freelists[size_class].next; cur; cur = cur->next)
		{
			if(cur->size_pa >= size_pa)
			{
				u8* p = (u8*)cur;
				const size_t remnant_pa = cur->size_pa - size_pa;

				freelist_remove(cur);

				if(remnant_pa)
					freelist_add(p+size_pa, remnant_pa);

				return p;
			}
		}

		return 0;
	}

	// returns 0 if there is no big enough entry in any freelist.
	void* alloc_from_larger_class(uint start_size_class, size_t size_pa)
	{
		uint classes_left = bitmap;
		// .. strip off all smaller classes
		classes_left &= (~0 << start_size_class);

		// for each non-empty freelist (loop doesn't incur overhead for
		// empty freelists)
		while(classes_left)
		{
			const uint class_size = ls1(classes_left);
			classes_left &= ~class_size;	// remove from classes_left
			const uint size_class = size_class_of(class_size);

			// .. try to alloc
			void* p = alloc_from_class(size_class, size_pa);
			if(p)
				return p;
		}

		// apparently all classes above start_size_class are empty,
		// or the above would have succeeded.
		debug_assert(bitmap < BIT(start_size_class+1));
		return 0;
	}

	//-------------------------------------------------------------------------
	// stats and validation
	size_t allocated_size_total_pa, free_size_total_pa;

	void stats_notify_alloc(size_t size_pa) { allocated_size_total_pa += size_pa; }
	void stats_notify_free(size_t size_pa) { free_size_total_pa += size_pa; }
	void stats_reset() { allocated_size_total_pa = free_size_total_pa = 0; }

	void self_check() const
	{
		debug_assert(allocated_size_total_pa+free_size_total_pa == pool.da.cur_size);

		// make sure freelists contain exactly free_size_total_pa bytes
		size_t freelist_size_total_pa = 0;
		uint classes_left = bitmap;
		while(classes_left)
		{
			const uint class_size = ls1(classes_left);
			classes_left &= ~class_size;	// remove from classes_left
			const uint size_class = size_class_of(class_size);
			for(const Header* p = &freelists[size_class]; p; p = p->next)
				freelist_size_total_pa += p->size_pa;
		}
		debug_assert(free_size_total_pa == freelist_size_total_pa);
	}
};	// CacheAllocator

static CacheAllocator cache_allocator;

//-----------------------------------------------------------------------------

/*
list of FileIOBufs currently held by the application.

note: "currently held" means between a file_buf_alloc/file_buf_retrieve
and file_buf_free.
additionally, the buffer may be stored in file_cache if file_cache_add
was called; it remains there until evicted in favor of another buffer.

rationale: users are strongly encouraged to access buffers as follows:
"alloc, use, free; alloc next..". this means only a few (typically one) are
active at a time. a list of these is more efficient to go through (O(1))
than having to scan file_cache for the buffer (O(N)).

see also discussion at declaration of FileIOBuf.
*/
class ExtantBufMgr
{
public:
	ExtantBufMgr()
		: extant_bufs(), epoch(1) {}

	// return index of ExtantBuf that contains <buf>, or -1.
	ssize_t find(FileIOBuf buf) const
	{
		debug_assert(buf != 0);
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			const ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
				return (ssize_t)i;
		}

		return -1;	// not found
	}

	// add given buffer to extant list.
	// long_lived indicates if this buffer will not be freed immediately
	// (more precisely: before allocating the next buffer); see FB_LONG_LIVED.
	// note: reuses a previous extant_bufs[] slot if one is unused.
	void add(FileIOBuf buf, size_t size, const char* atom_fn, uint fb_flags)
	{
		// cache_allocator also does this; we need to follow suit so that
		// matches() won't fail due to zero-length size.
		if(!size)
			size = 1;

		// don't do was-immediately-freed check for long_lived buffers.
		const bool long_lived = (fb_flags & FB_LONG_LIVED) != 0;
		const uint this_epoch = long_lived? 0 : epoch++;

		debug_assert(buf != 0);
		// look for holes in array and reuse those
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(eb.atom_fn == atom_fn)
				debug_warn("already exists!");
			// slot currently empty
			if(!eb.buf)
			{
				debug_assert(eb.refs == 0);
				eb.refs     = 1;
				eb.buf      = buf;
				eb.size     = size;
				eb.fb_flags = fb_flags;
				eb.atom_fn  = atom_fn;
				eb.epoch    = this_epoch;
				return;
			}
		}
		// add another entry
		extant_bufs.push_back(ExtantBuf(buf, size, fb_flags, atom_fn, this_epoch));
	}

	// indicate that a reference has been taken for <buf>;
	// parameters are the same as for add().
	void add_ref(FileIOBuf buf, size_t size, const char* atom_fn, bool long_lived)
	{
		ssize_t idx = find(buf);
		// this buf was already on the extant list
		if(idx != -1)
			extant_bufs[idx].refs++;
		// it was in cache and someone is 'reactivating' it, i.e. moving it
		// to the extant list.
		else
			add(buf, size, atom_fn, long_lived);
	}

	// return atom_fn that was passed when add()-ing this buf, or 0 if
	// it's not on extant list.
	const char* get_owner_filename(FileIOBuf buf)
	{
		ssize_t idx = find(buf);
		if(idx != -1)
			return extant_bufs[idx].atom_fn;
		else
			return 0;
	}

	// return false and warn if buf is not on extant list; otherwise,
	// pass back its size/owner filename and decrement reference count.
	// the return value indicates whether it reached 0, i.e. was
	// actually removed from the extant list.
	bool find_and_remove(FileIOBuf buf, size_t& size, const char*& atom_fn)
	{
		ssize_t idx = find(buf);
		if(idx == -1)
		{
			debug_warn("buf is not on extant list! double free?");
			return false;
		}

		ExtantBuf& eb = extant_bufs[idx];
		size      = eb.size;
		atom_fn   = eb.atom_fn;

		if(eb.epoch != 0 && eb.epoch != epoch-1)
			debug_warn("buf not released immediately");
		epoch++;

		bool actually_removed = false;
		// no more references
		if(--eb.refs == 0)
		{
			// mark slot in extant_bufs[] as reusable
			memset(&eb, 0, sizeof(eb));

			actually_removed = true;
		}

		return actually_removed;
	}

	// wipe out the entire list without freeing any FileIOBuf.
	// only meant to be used in file_cache_reset: since the allocator
	// is completely reset, there's no need to free outstanding items first.
	void clear()
	{
		extant_bufs.clear();
	}

	// if buf is not in extant list, complain; otherwise, mark it as
	// coming from the file <atom_fn>.
	// this is needed in the following case: uncompressed reads from archive
	// boil down to a file_io of the archive file. the buffer is therefore
	// tagged with the archive filename instead of the desired filename.
	// afile_read sets things right by calling this.
	void replace_owner(FileIOBuf buf, const char* atom_fn)
	{
		ssize_t idx = find(buf);
		if(idx != -1)
			extant_bufs[idx].atom_fn = atom_fn;
		else
			debug_warn("to-be-replaced buf not found");
	}

	// display list of all extant buffers in debug outut.
	// meant to be called at exit, at which time any remaining buffers
	// must apparently have been leaked.
	void display_all_remaining()
	{
		debug_printf("Leaked FileIOBufs:\n");
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(eb.buf)
				debug_printf("  %p (0x%08x) %s\n", eb.buf, eb.size, eb.atom_fn);
		}
		debug_printf("--------\n");
	}

private:
	struct ExtantBuf
	{
		// treat as user-visible padded buffer, although it may already be
		// the correct exact_buf.
		// rationale: file_cache_retrieve gets padded_buf from file_cache
		// and then calls add_ref. if not already in extant list, that
		// would be added, whereas file_buf_alloc's add() would specify
		// the exact_buf. assuming it's padded_buf is safe because
		// exact_buf_oracle can be used to get exact_buf from that.
		FileIOBuf buf;

		// treat as user-visible size, although it may already be the
		// correct exact_size.
		// rationale: this would also be available via TFile, but we want
		// users to be able to allocate file buffers (and they don't know tf).
		// therefore, we store this separately.
		size_t size;

		// FileBufFlags
		uint fb_flags;

		// which file was this buffer taken from?
		// we search for given atom_fn as part of file_cache_retrieve
		// (since we are responsible for already extant bufs).
		// also useful for tracking down buf 'leaks' (i.e. someone
		// forgetting to call file_buf_free).
		const char* atom_fn;

		// active references, i.e. how many times file_buf_free must be
		// called until this buffer is freed and removed from extant list.
		uint refs;

		// used to check if this buffer was freed immediately
		// (before allocating the next). that is the desired behavior
		// because it avoids fragmentation and leaks.
		uint epoch;

		ExtantBuf(FileIOBuf buf_, size_t size_, uint fb_flags_, const char* atom_fn_, uint epoch_)
			: buf(buf_), size(size_), fb_flags(fb_flags_), atom_fn(atom_fn_), refs(1), epoch(epoch_) {}
	};

	std::vector<ExtantBuf> extant_bufs;

	// see if buf (which may be padded) falls within eb's buffer.
	// this is necessary for file_buf_free; we do not know the size
	// of buffer to free until after find_and_remove, so exact_buf_oracle
	// cannot be used.
	bool matches(const ExtantBuf& eb, FileIOBuf buf) const
	{
		return (eb.buf <= buf && buf < (u8*)eb.buf+eb.size);
	}

	uint epoch;
};	// ExtantBufMgr
static ExtantBufMgr extant_bufs;

//-----------------------------------------------------------------------------

// HACK: key type is really const char*, but the file_cache's STL (hash_)map
// stupidly assumes that is a "string". (comparison can be done via
// pointer compare, due to atom_fn mechanism) we define as void* to avoid
// this behavior - it breaks the (const char*)1 self-test hack and is
// inefficient.
static Cache<const void*, FileIOBuf> file_cache;

/*
mapping of padded_buf to the original exact_buf and exact_size.

rationale: cache stores the user-visible (padded) buffer, but we need
to pass the original to cache_allocator.
since not all buffers end up padded (only happens if reading
uncompressed files from archive), it is more efficient to only
store bookkeeping information for those who need it (rather than
maintaining a complete list of allocs in cache_allocator).

storing both padded and exact buf/size in a FileIOBuf struct is not really
an option: that begs the question how users initialize it, and can't
well be stored in Cache.
*/
class ExactBufOracle
{
public:
	typedef std::pair<FileIOBuf, size_t> BufAndSize;

	// associate padded_buf with exact_buf and exact_size;
	// these can later be retrieved via get().
	// should only be called if necessary, i.e. they are not equal.
	// assumes and verifies that the association didn't already exist
	// (otherwise it's a bug, because it's removed when buf is freed)
	void add(FileIOBuf exact_buf, size_t exact_size, FileIOBuf padded_buf)
	{
		debug_assert((uintptr_t)exact_buf % BUF_ALIGN == 0);
		debug_assert(exact_buf <= padded_buf);

		std::pair<Padded2Exact::iterator, bool> ret;
		const BufAndSize item = std::make_pair(exact_buf, exact_size);
		ret = padded2exact.insert(std::make_pair(padded_buf, item));
		// make sure it wasn't already in the map
		debug_assert(ret.second == true);
	}

	// return exact_buf and exact_size that were associated with <padded_buf>.
	// can optionally remove that association afterwards (slightly more
	// efficient than a separate remove() call).
	BufAndSize get(FileIOBuf padded_buf, size_t size, bool remove_afterwards = false)
	{
		Padded2Exact::iterator it = padded2exact.find(padded_buf);

		BufAndSize ret;
		// not found => must already be exact_buf. will be verified below.
		if(it == padded2exact.end())
			ret = std::make_pair(padded_buf, size);
		else
		{
			ret = it->second;

			// something must be different, else it shouldn't have been
			// added anyway.
			// actually, no: file_io may have had to register these values
			// (since its user_size != size), but they may match what
			// caller passed us.
			//debug_assert(ret.first != padded_buf || ret.second != size);

			if(remove_afterwards)
				padded2exact.erase(it);
		}

		// exact_buf must be aligned, or something is wrong.
		debug_assert((uintptr_t)ret.first  % BUF_ALIGN == 0);
		return ret;
	}

	// remove all associations. this is intended only for use in
	// file_cache_reset.
	void clear()
	{
		padded2exact.clear();
	}

private:
	typedef std::map<FileIOBuf, BufAndSize> Padded2Exact;
	Padded2Exact padded2exact;
};
static ExactBufOracle exact_buf_oracle;

// referenced by cache_alloc
static void free_padded_buf(FileIOBuf padded_buf, size_t size, bool from_heap = false);

static void cache_free(FileIOBuf exact_buf, size_t exact_size)
{
	cache_allocator.dealloc((u8*)exact_buf, exact_size);
}

static FileIOBuf cache_alloc(size_t size)
{
	uint attempts = 0;
	for(;;)
	{
		FileIOBuf buf = (FileIOBuf)cache_allocator.alloc(size);
		if(buf)
			return buf;

		// remove least valuable entry from cache and free its buffer.
		FileIOBuf discarded_buf; size_t size;
		bool removed = file_cache.remove_least_valuable(&discarded_buf, &size);
		// only false if cache is empty, which can't be the case because
		// allocation failed.
		debug_assert(removed);

		// discarded_buf may be the least valuable entry in cache, but if
		// still in use (i.e. extant), it must not actually be freed yet!
		if(extant_bufs.find(discarded_buf) == -1)
		{
			free_padded_buf(discarded_buf, size);

			// optional: this iteration doesn't really count because no
			// memory was actually freed. helps prevent infinite loop
			// warning without having to raise the limit really high.
			attempts--;
		}

		// note: this may seem hefty, but 300 is known to be reached.
		// (after building archive, file cache is full; attempting to
		// allocate ~4MB while only freeing small blocks scattered over
		// the entire cache can take a while)
		if(++attempts > 500)
			debug_warn("possible infinite loop: failed to make room in cache");
	}

	UNREACHABLE;
}


// translate <padded_buf> to the exact buffer and free it.
// convenience function used by file_buf_alloc and file_buf_free.
static void free_padded_buf(FileIOBuf padded_buf, size_t size, bool from_heap)
{
	const bool remove_afterwards = true;
	ExactBufOracle::BufAndSize exact = exact_buf_oracle.get(padded_buf, size, remove_afterwards);
	FileIOBuf exact_buf = exact.first; size_t exact_size = exact.second;

	if(from_heap)
		page_aligned_free((void*)exact_buf, exact_size);
	else
		cache_free(exact_buf, exact_size);
}


// allocate a new buffer of <size> bytes (possibly more due to internal
// fragmentation). never returns 0.
// <atom_fn>: owner filename (buffer is intended to be used for data from
//   this file).
// <fb_flags>: see FileBufFlags.
FileIOBuf file_buf_alloc(size_t size, const char* atom_fn, uint fb_flags)
{
	const bool should_update_stats = (fb_flags & FB_NO_STATS) == 0;
	const bool from_heap           = (fb_flags & FB_FROM_HEAP) != 0;

	FileIOBuf buf;
	if(from_heap)
	{
		buf = (FileIOBuf)page_aligned_alloc(size);
		if(!buf)
			WARN_ERR(ERR::NO_MEM);
	}
	else
		buf = cache_alloc(size);

	extant_bufs.add(buf, size, atom_fn, fb_flags);

	if(should_update_stats)
		stats_buf_alloc(size, round_up(size, BUF_ALIGN));
	return buf;
}


// mark <buf> as no longer needed. if its reference count drops to 0,
// it will be removed from the extant list. if it had been added to the
// cache, it remains there until evicted in favor of another buffer.
LibError file_buf_free(FileIOBuf buf, uint fb_flags)
{
	const bool should_update_stats = (fb_flags & FB_NO_STATS) == 0;
	const bool from_heap           = (fb_flags & FB_FROM_HEAP) != 0;

	if(!buf)
		return INFO::OK;

	size_t size; const char* atom_fn;
	bool actually_removed = extant_bufs.find_and_remove(buf, size, atom_fn);
	if(actually_removed)
	{
		// avoid any potential confusion and some overhead by skipping the
		// retrieve step (not needed anyway).
		if(from_heap)
			goto free_immediately;

		{
		FileIOBuf buf_in_cache;
		// it's still in cache - leave its buffer intact.
		if(file_cache.retrieve(atom_fn, buf_in_cache, 0, false))
		{
			// sanity checks: what's in cache must match what we have.
			// note: don't compare actual_size with cached size - they are
			// usually different.
			debug_assert(buf_in_cache == buf);
		}
		// buf is not in cache - needs to be freed immediately.
		else
		{
free_immediately:
			// note: extant_bufs cannot be relied upon to store and return
			// exact_buf - see definition of ExtantBuf.buf.
			// we have to use exact_buf_oracle, which is a bit slow, but hey.
			free_padded_buf(buf, size, from_heap);
		}
		}
	}

	if(should_update_stats)
		stats_buf_free();
	trace_notify_free(atom_fn, size);

	return INFO::OK;
}


// inform us that the buffer address will be increased by <padding>-bytes.
// this happens when reading uncompressed files from archive: they
// start at unaligned offsets and file_io rounds offset down to
// next block boundary. the buffer therefore starts with padding, which
// is skipped so the user only sees their data.
// we make note of the new buffer address so that it can be freed correctly
// by passing the new padded buffer.
void file_buf_add_padding(FileIOBuf exact_buf, size_t exact_size, size_t padding)
{
	debug_assert(padding < FILE_BLOCK_SIZE);
	FileIOBuf padded_buf = (FileIOBuf)((u8*)exact_buf + padding);
	exact_buf_oracle.add(exact_buf, exact_size, padded_buf);
}


// if buf is not in extant list, complain; otherwise, mark it as
// coming from the file <atom_fn>.
// this is needed in the following case: uncompressed reads from archive
// boil down to a file_io of the archive file. the buffer is therefore
// tagged with the archive filename instead of the desired filename.
// afile_read sets things right by calling this.
LibError file_buf_set_real_fn(FileIOBuf buf, const char* atom_fn)
{
	// note: removing and reinserting would be easiest, but would
	// mess up the epoch field.
	extant_bufs.replace_owner(buf, atom_fn);
	return INFO::OK;
}


// if file_cache_add-ing the given buffer, would it be added?
// this is referenced by trace_entry_causes_io; see explanation there.
bool file_cache_would_add(size_t size, const char* UNUSED(atom_fn),
	uint file_flags)
{
	// caller is saying this file shouldn't be cached here.
	if(file_flags & FILE_CACHED_AT_HIGHER_LEVEL)
		return false;

	// refuse to cache 0-length files (it would have no benefit and
	// causes problems due to divide-by-0).
	if(size == 0)
		return false;

	return true;
}


// "give" <buf> to the cache, specifying its size and owner filename.
// since this data may be shared among users of the cache, it is made
// read-only (via MMU) to make sure no one can corrupt/change it.
//
// note: the reference added by file_buf_alloc still exists! it must
// still be file_buf_free-d after calling this.
LibError file_cache_add(FileIOBuf buf, size_t size, const char* atom_fn,
	uint file_flags)
{
	debug_assert(buf);

	if(!file_cache_would_add(size, atom_fn, file_flags))
		return INFO::SKIPPED;

	// assign cost
	uint cost = 1;

	ExactBufOracle::BufAndSize bas = exact_buf_oracle.get(buf, size);
	FileIOBuf exact_buf = bas.first; size_t exact_size = bas.second;
	cache_allocator.make_read_only((u8*)exact_buf, exact_size);

	file_cache.add(atom_fn, buf, size, cost);

	return INFO::OK;
}




// check if the contents of the file <atom_fn> are in file cache.
// if not, return 0; otherwise, return buffer address and optionally
// pass back its size.
//
// note: does not call stats_cache because it does not know the file size
// in case of cache miss! doing so is left to the caller.
FileIOBuf file_cache_retrieve(const char* atom_fn, size_t* psize, uint fb_flags)
{
	// note: do not query extant_bufs - reusing that doesn't make sense
	// (why would someone issue a second IO for the entire file while
	// still referencing the previous instance?)

	const bool long_lived = (fb_flags & FB_LONG_LIVED) != 0;
	const bool should_account = (fb_flags & FB_NO_ACCOUNTING) == 0;
	const bool should_update_stats = (fb_flags & FB_NO_STATS) == 0;

	FileIOBuf buf;
	const bool should_refill_credit = should_account;
	if(!file_cache.retrieve(atom_fn, buf, psize, should_refill_credit))
		return 0;

	if(should_account)
		extant_bufs.add_ref(buf, *psize, atom_fn, long_lived);

	if(should_update_stats)
		stats_buf_ref();

	return buf;
}


// invalidate all data loaded from the file <fn>. this ensures the next
// load of this file gets the (presumably new) contents of the file,
// not previous stale cache contents.
// call after hotloading code detects file has been changed.
LibError file_cache_invalidate(const char* P_fn)
{
	const char* atom_fn = file_make_unique_fn_copy(P_fn);

	// note: what if the file has an extant buffer?
	// this *could* conceivably happen during hotloading if a file is
	// saved right when the engine wants to access it (unlikely but not
	// impossible).
	// what we'll do is just let them continue as if nothing had happened;
	// invalidating is only meant to make sure that the reload's IO
	// will load the new data (not stale stuff from cache).
	// => nothing needs to be done.

	// mark all blocks from the file as invalid
	block_mgr.invalidate(atom_fn);

	// file was cached: remove it and free that memory
	FileIOBuf cached_buf; size_t size;
	if(file_cache.retrieve(atom_fn, cached_buf, &size))
	{
		file_cache.remove(atom_fn);
		free_padded_buf(cached_buf, size);
	}

	return INFO::OK;
}


// reset entire state of the file cache to what it was after initialization.
// that means completely emptying the extant list and cache.
// used after simulating cache operation, which fills the cache with
// invalid data.
void file_cache_reset()
{
	// just wipe out extant list and cache without freeing the bufs -
	// cache allocator is completely reset below.

	extant_bufs.clear();

	// note: do not loop until file_cache.empty - there may still be
	// some items pending eviction even though cache is "empty".
	FileIOBuf discarded_buf; size_t size;
	while(file_cache.remove_least_valuable(&discarded_buf, &size))
	{
	}

	cache_allocator.reset();
	exact_buf_oracle.clear();
}



void file_cache_init()
{
}


void file_cache_shutdown()
{
	extant_bufs.display_all_remaining();
	cache_allocator.shutdown();
	block_mgr.shutdown();
}


// for self test

void* file_cache_allocator_alloc(size_t size)
{
	return cache_allocator.alloc(size);
}
void file_cache_allocator_free(u8* p, size_t size)
{
	return cache_allocator.dealloc(p, size);
}
void file_cache_allocator_reset()
{
	cache_allocator.reset();
}
