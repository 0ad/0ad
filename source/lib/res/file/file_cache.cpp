#include "precompiled.h"

#include <map>

#include "lib/allocators.h"
#include "lib/byte_order.h"
#include "lib/adts.h"
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

// >= AIO_SECTOR_SIZE or else waio will have to realign.
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
static const size_t MAX_CACHE_SIZE = 64*MiB;
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
		const size_t size_pa = round_up(size, BUF_ALIGN);
		void* p;

		// try to reuse a freed entry
		const uint size_class = size_class_of(size_pa);
		p = alloc_from_class(size_class, size_pa);
		if(p)
			return p;

		// grab more space from pool
		p = pool_alloc(&pool, size_pa);
		if(p)
			return p;

		// last resort: split a larger element
		p = alloc_from_larger_class(size_class, size_pa);
		if(p)
			return p;

		// failed - can no longer expand and nothing big enough was
		// found in freelists.
		// file cache will decide which elements are least valuable,
		// free() those and call us again.
		return 0;
	}

	void make_read_only(u8* p, size_t size)
	{
		const size_t size_pa = round_up(size, BUF_ALIGN);
		(void)mprotect(p, size_pa, PROT_READ);
	}

#include "nommgr.h"
	void free(u8* p, size_t size)
#include "mmgr.h"
	{
		// make sure entire range is within pool.
		if(!pool_contains(&pool, p) || !pool_contains(&pool, p+size-1))
		{
			debug_warn("invalid pointer");
			return;
		}

		size_t size_pa = round_up(size, BUF_ALIGN);

		// (re)allow writes
		//
		// note: unfortunately we cannot unmap this buffer's memory
		// (to make sure it is not used) because we write a header/footer
		// into it to support coalescing.
		(void)mprotect(p, size_pa, PROT_READ|PROT_WRITE);

		coalesce(p, size_pa);
		freelist_add(p, size_pa);
	}

	// free all allocations and reset state to how it was just after
	// (the first and only) init() call.
	void reset()
	{
		pool_free_all(&pool);
		bitmap = 0;
		memset(freelists, 0, sizeof(freelists));
	}

private:
	Pool pool;

	uint size_class_of(size_t size_pa)
	{
		return log2((uint)size_pa);
	}

	//-------------------------------------------------------------------------
	// boundary tags for coalescing
	static const u32 MAGIC1 = FOURCC('C','M','E','M');
	static const u32 MAGIC2 = FOURCC('\x00','\xFF','\x55','\xAA');
	struct FreePage
	{
		FreePage* prev;
		FreePage* next;
		size_t size_pa;
		u32 magic1;
		u32 magic2;
	};
	// must be enough room to stash 2 FreePage instances in the freed page.
	cassert(BUF_ALIGN >= 2*sizeof(FreePage));

	// check if there is a free allocation before/after <p>.
	// return 0 if not, otherwise a pointer to its FreePage header/footer.
	// if ofs = 0, check before; otherwise, it gives the size of the
	// current allocation, and we check behind that.
	// notes:
	// - p and ofs are trusted: [p, p+ofs) lies within the pool.
	// - correctly deals with p lying at start/end of pool.
	FreePage* freed_page_at(u8* p, size_t ofs)
	{
		// checking the footer of the memory before p.
		if(!ofs)
		{
			// .. but p is at front of pool - bail.
			if(p == pool.da.base)
				return 0;
			p -= sizeof(FreePage);
		}
		// checking header of memory after p+ofs.
		else
		{
			p += ofs;
			// .. but it's at end of the currently committed region - bail.
			if(p >= pool.da.base+pool.da.cur_size)
				return 0;
		}

		// check if there is a valid FreePage header/footer at p.
		// we use magic values to differentiate the header from user data
		// (this isn't 100% reliable, but we can't insert extra boundary
		// tags because the memory must remain aligned).
		FreePage* page = (FreePage*)p;
		if(page->magic1 != MAGIC1 || page->magic2 != MAGIC2)
			return 0;
		debug_assert(page->size_pa % BUF_ALIGN == 0);
		return page;
	}

	// check if p's neighbors are free; if so, merges them all into
	// one big region and updates freelists accordingly.
	// p and size_pa are trusted: [p, p+size_pa) lies within the pool.
	void coalesce(u8*& p, size_t& size_pa)
	{
		FreePage* prev = freed_page_at(p, 0);
		if(prev)
		{
			freelist_remove(prev);
			p -= prev->size_pa;
			size_pa += prev->size_pa;
		}
		FreePage* next = freed_page_at(p, size_pa);
		if(next)
		{
			freelist_remove(next);
			size_pa += next->size_pa;
		}
	}

	//-------------------------------------------------------------------------
	// freelist
	uintptr_t bitmap;
	FreePage* freelists[sizeof(uintptr_t)*CHAR_BIT];

	void freelist_add(u8* p, size_t size_pa)
	{
		const uint size_class = size_class_of(size_pa);

		// write header and footer into the freed mem
		// (its prev and next link fields will be set below)
		FreePage* header = (FreePage*)p;
		header->prev = header->next = 0;
		header->size_pa = size_pa;
		header->magic1 = MAGIC1; header->magic2 = MAGIC2;
		FreePage* footer = (FreePage*)(p+size_pa-sizeof(FreePage));
		*footer = *header;

		// insert the header into freelist
		// .. list was empty: link to head
		if(!freelists[size_class])
		{
			freelists[size_class] = header;
			bitmap |= BIT(size_class);
		}
		// .. not empty: link to node (address order)
		else
		{
			FreePage* prev = freelists[size_class];
			// find node to insert after
			while(prev->next && header <= prev->next)
				prev = prev->next;
			header->next = prev->next;
			header->prev = prev;
		}
	}

	void freelist_remove(FreePage* page)
	{
		const uint size_class = size_class_of(page->size_pa);

		// in middle of list: unlink from prev node
		if(page->prev)
			page->prev->next = page->next;
		// was at front of list: unlink from head
		else
		{
			freelists[size_class] = page->next;
			// freelist is now empty - update bitmap.
			if(!page->next)
				bitmap &= ~BIT(size_class);
		}

		// not at end of list: unlink from next node
		if(page->next)
			page->next->prev = page->prev;
	}

	void* alloc_from_class(uint size_class, size_t size_pa)
	{
		// return first suitable entry in (address-ordered) list
		FreePage* cur = freelists[size_class];
		while(cur)
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
			cur = cur->next;
		}

		return 0;
	}

	void* alloc_from_larger_class(uint start_size_class, size_t size_pa)
	{
		uint classes_left = bitmap;
		// .. strip off all smaller classes
		classes_left &= (~0 << start_size_class);
		while(classes_left)
		{
#define LS1(x) (x & -(int)x)	// value of LSB 1-bit
			const uint class_size = LS1(classes_left);
			classes_left &= ~class_size;	// remove from classes_left
			const uint size_class = size_class_of(class_size);
			void* p = alloc_from_class(size_class, size_pa);
			if(p)
				return p;
		}

		// apparently all classes above start_size_class are empty,
		// or the above would have succeeded.
		debug_assert(bitmap < BIT(start_size_class+1));
		return 0;
	}
};	// CacheAllocator

static CacheAllocator cache_allocator;

//-----------------------------------------------------------------------------

// list of FileIOBufs currently held by the application.
class ExtantBufMgr
{
	struct ExtantBuf
	{
		FileIOBuf buf;
		// this would also be available via TFile, but we want users
		// to be able to allocate file buffers (and they don't know tf).
		// therefore, we store this separately.
		size_t size;
		// which file was this buffer taken from?
		// we search for given atom_fn as part of file_cache_retrieve
		// (since we are responsible for already extant bufs).
		// also useful for tracking down buf 'leaks' (i.e. someone
		// forgetting to call file_buf_free).
		const char* atom_fn;
		//
		uint refs;
		// used to check if this buffer was freed immediately
		// (before allocating the next). that is the desired behavior
		// because it avoids fragmentation and leaks.
		uint epoch;
		ExtantBuf(FileIOBuf buf_, size_t size_, const char* atom_fn_, uint epoch_)
			: buf(buf_), size(size_), atom_fn(atom_fn_), refs(1), epoch(epoch_) {}
	};
	std::vector<ExtantBuf> extant_bufs;

public:
	ExtantBufMgr()
		: extant_bufs(), epoch(1) {}

	void add(FileIOBuf buf, size_t size, const char* atom_fn, bool long_lived)
	{
		// don't do was-immediately-freed check for long_lived buffers.
		const uint this_epoch = long_lived? 0 : epoch++;

		debug_assert(buf != 0);
		// look for holes in array and reuse those
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(!eb.buf)
			{
				debug_assert(eb.refs == 0);
				eb.refs    = 1;
				eb.buf     = buf;
				eb.size    = size;
				eb.atom_fn = atom_fn;
				eb.epoch   = this_epoch;
				return;
			}
		}
		// add another entry
		extant_bufs.push_back(ExtantBuf(buf, size, atom_fn, this_epoch));
	}

	void add_ref(FileIOBuf buf, size_t size, const char* atom_fn)
	{
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
			{
				eb.refs++;
				return;
			}
		}

		add(buf, size, atom_fn, 0);
	}

	const char* get_owner_filename(FileIOBuf buf)
	{
		debug_assert(buf != 0);
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
				return eb.atom_fn;
		}
		return 0;
	}

	void find_and_remove(FileIOBuf buf, size_t* size, const char** atom_fn)
	{
		debug_assert(buf != 0);
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
			{
				*size = eb.size;
				*atom_fn = eb.atom_fn;

				if(--eb.refs == 0)
				{
					// mark as reusable
					eb.buf     = 0;
					eb.size    = 0;
					eb.atom_fn = 0;
				}

				if(eb.epoch != 0 && eb.epoch != epoch-1)
					debug_warn("buf not released immediately");
				epoch++;
				return;
			}
		}

		debug_warn("buf is not on extant list! double free?");
	}

	void replace_owner(FileIOBuf buf, const char* atom_fn)
	{
		debug_assert(buf != 0);
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
			{
				eb.atom_fn = atom_fn;
				return;
			}
		}

		debug_warn("to-be-replaced buf not found");
	}

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
	bool matches(ExtantBuf& eb, FileIOBuf buf)
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


FileIOBuf file_buf_alloc(size_t size, const char* atom_fn, bool long_lived)
{
	FileIOBuf buf;

	uint attempts = 0;
	for(;;)
	{
		buf = (FileIOBuf)cache_allocator.alloc(size);
		if(buf)
			break;

		// tell file_cache to remove some items. this may shake loose
		// several, all of which must be returned to the cache_allocator.
		for(;;)
		{
			size_t size;
			FileIOBuf discarded_buf = file_cache.remove_least_valuable(&size);
			if(!discarded_buf)
				break;
#include "nommgr.h"
			cache_allocator.free((u8*)discarded_buf, size);
#include "mmgr.h"
		}

		if(attempts++ > 50)
			debug_warn("possible infinite loop: failed to make room in cache");
	}

	extant_bufs.add(buf, size, atom_fn, long_lived);

	stats_buf_alloc(size, round_up(size, BUF_ALIGN));
	return buf;
}


LibError file_buf_get(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, uint flags, FileIOCB cb)
{
	// decode *pbuf - exactly one of these is true
	const bool temp  = (pbuf == FILE_BUF_TEMP);
	const bool alloc = !temp && (*pbuf == FILE_BUF_ALLOC);
	const bool user  = !temp && !alloc;

	const bool is_write = (flags & FILE_WRITE) != 0;
	const bool long_lived = (flags & FILE_LONG_LIVED) != 0;

	// reading into temp buffers - ok.
	if(!is_write && temp && cb != 0)
		return ERR_OK;

	// reading and want buffer allocated.
	if(!is_write && alloc)
	{
		*pbuf = file_buf_alloc(size, atom_fn, long_lived);
		if(!*pbuf)	// very unlikely (size totally bogus or cache hosed)
			WARN_RETURN(ERR_NO_MEM);
		return ERR_OK;
	}

	// writing from user-specified buffer - ok
	if(is_write && user)
		return ERR_OK;

	WARN_RETURN(ERR_INVALID_PARAM);
}


LibError file_buf_free(FileIOBuf buf)
{
	if(!buf)
		return ERR_OK;

	size_t size; const char* atom_fn;
	extant_bufs.find_and_remove(buf, &size, &atom_fn);

	stats_buf_free();
	trace_notify_free(atom_fn, size);

	return ERR_OK;
}


// mark <buf> as belonging to the file <atom_fn>. this is done after
// reading uncompressed data from archive: file_io.cpp must allocate the
// buffer, since only it knows how much padding is needed; however,
// archive.cpp knows the real filename (as opposed to that of the archive,
// which is what the file buffer is associated with). therefore,
// we fix up the filename afterwards.
LibError file_buf_set_real_fn(FileIOBuf buf, const char* atom_fn)
{
	// note: removing and reinserting would be easiest, but would
	// mess up the epoch field.
	extant_bufs.replace_owner(buf, atom_fn);
	return ERR_OK;
}




LibError file_cache_add(FileIOBuf buf, size_t size, const char* atom_fn)
{
	// decide (based on flags) if buf is to be cached; set cost
	uint cost = 1;

	if(buf)
		cache_allocator.make_read_only((u8*)buf, size);
	file_cache.add(atom_fn, buf, size, cost);

	return ERR_OK;
}





// called by trace simulator to retrieve buffer address, given atom_fn.
// must not change any cache state (e.g. notify stats or add ref).
FileIOBuf file_cache_find(const char* atom_fn, size_t* size)
{
	return file_cache.retrieve(atom_fn, size, false);
}


FileIOBuf file_cache_retrieve(const char* atom_fn, size_t* psize)
{
	// note: do not query extant_bufs - reusing that doesn't make sense
	// (why would someone issue a second IO for the entire file while
	// still referencing the previous instance?)

	FileIOBuf buf = file_cache_find(atom_fn, psize);
	if(buf)
	{
		extant_bufs.add_ref(buf, *psize, atom_fn);
		stats_buf_ref();
	}

	CacheRet cr = buf? CR_HIT : CR_MISS;
	stats_cache(cr, *psize, atom_fn);

	return buf;
}


/*
a) FileIOBuf is opaque type with getter
FileIOBuf buf;	<--------------------- how to initialize??
file_io(.., &buf);
data = file_buf_contents(&buf);
file_buf_free(&buf);

would obviate lookup struct but at expense of additional getter and
trouble with init - need to set FileIOBuf to wrap user's buffer, or
only allow us to return buffer address (which is ok)

b) FileIOBuf is pointer to the buf, and secondary map associates that with BufInfo
FileIOBuf buf;
file_io(.., &buf);
file_buf_free(&buf);

secondary map covers all currently open IO buffers. it is accessed upon
file_buf_free and there are only a few active at a time ( < 10)

*/










// remove all blocks loaded from the file <fn>. used when reloading the file.
LibError file_cache_invalidate(const char* P_fn)
{
	const char* atom_fn = file_make_unique_fn_copy(P_fn);

	// mark all blocks from the file as invalid
	block_mgr.invalidate(atom_fn);

	// file was cached: remove it and free that memory
	size_t size;
	FileIOBuf cached_buf = file_cache.retrieve(atom_fn, &size);
	if(cached_buf)
	{
		file_cache.remove(atom_fn);
#include "nommgr.h"
		cache_allocator.free((u8*)cached_buf, size);
#include "mmgr.h"
	}

	return ERR_OK;
}


void file_cache_flush()
{
	for(;;)
	{
		size_t size;
		FileIOBuf discarded_buf = file_cache.remove_least_valuable(&size);
		// cache is now empty - done
		if(!discarded_buf)
			return;
#include "nommgr.h"
		cache_allocator.free((u8*)discarded_buf, size);
#include "mmgr.h"
	}
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


//-----------------------------------------------------------------------------
// built-in self test
//-----------------------------------------------------------------------------

#if SELF_TEST_ENABLED
namespace test {

static void test_cache_allocator()
{
	// allocated address -> its size
	typedef std::map<void*, size_t> AllocMap;
	AllocMap allocations;

	// put allocator through its paces by allocating several times
	// its capacity (this ensures memory is reused)
	srand(1);
	size_t total_size_used = 0;
	while(total_size_used < 4*MAX_CACHE_SIZE)
	{
		size_t size = rand(1, 10*MiB);
		total_size_used += size;
		void* p;
		// until successful alloc:
		for(;;)
		{
			p = cache_allocator.alloc(size);
			if(p)
				break;
			// out of room - remove a previous allocation
			// .. choose one at random
			size_t chosen_idx = (size_t)rand(0, (uint)allocations.size());
			AllocMap::iterator it = allocations.begin();
			for(; chosen_idx != 0; chosen_idx--)
				++it;
#include "nommgr.h"
			cache_allocator.free((u8*)it->first, it->second);
#include "mmgr.h"
			allocations.erase(it);
		}

		// must not already have been allocated
		TEST(allocations.find(p) == allocations.end());
		allocations[p] = size;
	}

	// reset to virginal state
	cache_allocator.reset();

}

static void test_file_cache()
{
	// we need a unique address for file_cache_add, but don't want to
	// actually put it in the atom_fn storage (permanently clutters it).
	// just increment this pointer (evil but works since it's not used).
//	const char* atom_fn = (const char*)1;

	// give to file_cache
//	file_cache_add((FileIOBuf)p, size, atom_fn++);

	file_cache_flush();
	TEST(file_cache.empty());

	// (even though everything has now been freed,
	// the freelists may be a bit scattered already).
}

static void self_test()
{
	test_cache_allocator();
	test_file_cache();
}

SELF_TEST_RUN;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED
