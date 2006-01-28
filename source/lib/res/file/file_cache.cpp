#include "precompiled.h"

#include <map>

#include "lib/allocators.h"
#include "lib/byte_order.h"
#include "lib/adts.h"
#include "file_internal.h"

// strategy:
// policy:
// - allocation: use all available mem first, then look at freelist
// - freelist: good fit, address-ordered, always split
// - free: immediately coalesce
// mechanism:
// - coalesce: boundary tags in freed memory
// - freelist: 2**n segregated doubly-linked, address-ordered
class CacheAllocator
{
	static const size_t MAX_CACHE_SIZE = 64*MiB;

public:
	void init()
	{
		// note: do not call from ctor; pool_create currently (2006-20-01)
		// breaks if called at NLSO init time.
		(void)pool_create(&pool, MAX_CACHE_SIZE, 0);
	}

	void shutdown()
	{
		(void)pool_destroy(&pool);
	}

	void* alloc(size_t size)
	{
		const size_t size_pa = round_up(size, AIO_SECTOR_SIZE);

		// use all available space first
		void* p = pool_alloc(&pool, size_pa);
		if(p)
			return p;

		// try to reuse a freed entry
		const uint size_class = size_class_of(size_pa);
		p = alloc_from_class(size_class, size_pa);
		if(p)
			return p;
		p = alloc_from_larger_class(size_class, size_pa);
		if(p)
			return p;

		// failed - can no longer expand and nothing big enough was
		// found in freelists.
		// file cache will decide which elements are least valuable,
		// free() those and call us again.
		return 0;
	}

#include "nommgr.h"
	void free(u8* p, size_t size)
#include "mmgr.h"
	{
		if(!pool_contains(&pool, p))
		{
			debug_warn("not in arena");
			return;
		}
		size_t size_pa = round_up(size, AIO_SECTOR_SIZE);

		coalesce(p, size_pa);
		freelist_add(p, size_pa);
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
	// must be enough room to stash header+footer in the freed page.
	cassert(AIO_SECTOR_SIZE >= 2*sizeof(FreePage));

	FreePage* freed_page_at(u8* p, size_t ofs)
	{
		if(!ofs)
			p -= sizeof(FreePage);
		else
			p += ofs;

		FreePage* page = (FreePage*)p;
		if(page->magic1 != MAGIC1 || page->magic2 != MAGIC2)
			return 0;
		debug_assert(page->size_pa % AIO_SECTOR_SIZE == 0);
		return page;
	}

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
					freelist_add(p+remnant_pa, remnant_pa);

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
			uint size_class = LS1(classes_left);
			classes_left &= ~BIT(size_class);	// remove from classes_left
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
		ExtantBuf(FileIOBuf buf_, size_t size_, const char* atom_fn_)
			: buf(buf_), size(size_), atom_fn(atom_fn_) {}
	};
	std::vector<ExtantBuf> extant_bufs;

public:
	void add(FileIOBuf buf, size_t size, const char* atom_fn)
	{
		debug_assert(buf != 0);
		// look for holes in array and reuse those
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(!eb.buf)
			{
				eb.buf     = buf;
				eb.size    = size;
				eb.atom_fn = atom_fn;
				return;
			}
		}
		// add another entry
		extant_bufs.push_back(ExtantBuf(buf, size, atom_fn));
	}

	bool includes(FileIOBuf buf)
	{
		debug_assert(buf != 0);
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
				return true;
		}
		return false;
	}

	void find_and_remove(FileIOBuf buf, size_t* size)
	{
		debug_assert(buf != 0);
		for(size_t i = 0; i < extant_bufs.size(); i++)
		{
			ExtantBuf& eb = extant_bufs[i];
			if(matches(eb, buf))
			{
				*size = eb.size;
				eb.buf     = 0;
				eb.size    = 0;
				eb.atom_fn = 0;
				return;
			}
		}

		debug_warn("buf is not on extant list! double free?");
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
};	// ExtantBufMgr
static ExtantBufMgr extant_bufs;

//-----------------------------------------------------------------------------


static Cache<const char*, FileIOBuf> file_cache;


FileIOBuf file_buf_alloc(size_t size, const char* atom_fn)
{
	FileIOBuf buf;

	uint attempts = 0;
	for(;;)
	{
		buf = (FileIOBuf)cache_allocator.alloc(size);
		if(buf)
			break;

		size_t size;
		FileIOBuf discarded_buf = file_cache.remove_least_valuable(&size);
#include "nommgr.h"
		if(discarded_buf)
			cache_allocator.free((u8*)discarded_buf, size);
#include "mmgr.h"

		if(attempts++ > 50)
			debug_warn("possible infinite loop: failed to make room in cache");
	}

	extant_bufs.add(buf, size, atom_fn);

	stats_buf_alloc(size, round_up(size, AIO_SECTOR_SIZE));
	return buf;
}


LibError file_buf_get(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, bool is_write, FileIOCB cb)
{
	// decode *pbuf - exactly one of these is true
	const bool temp  = (pbuf == FILE_BUF_TEMP);
	const bool alloc = !temp && (*pbuf == FILE_BUF_ALLOC);
	const bool user  = !temp && !alloc;

	// reading into temp buffers - ok.
	if(!is_write && temp && cb != 0)
		return ERR_OK;

	// reading and want buffer allocated.
	if(!is_write && alloc)
	{
		*pbuf = file_buf_alloc(size, atom_fn);
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

	stats_buf_free();

	size_t size;
	extant_bufs.find_and_remove(buf, &size);
	return ERR_OK;
}


LibError file_cache_add(FileIOBuf buf, size_t size, const char* atom_fn)
{
	// decide (based on flags) if buf is to be cached; set cost
	uint cost = 1;

	file_cache.add(atom_fn, buf, size, cost);

	return ERR_OK;
}


FileIOBuf file_cache_retrieve(const char* atom_fn, size_t* size)
{
	// note: do not query extant_bufs - reusing that doesn't make sense
	// (why would someone issue a second IO for the entire file while
	// still referencing the previous instance?)

	return file_cache.retrieve(atom_fn, size);
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
		void* mem;
		BlockStatus status;

		Block() {}	// for RingBuf
		Block(BlockId id_, void* mem_)
			: id(id_), mem(mem_), status(BS_PENDING) {}
	};
	RingBuf<Block, MAX_BLOCKS> blocks;
	typedef RingBuf<Block, MAX_BLOCKS>::iterator BlockIt;

	// use Pool to allocate mem for all blocks because it guarantees
	// page alignment (required for IO) and obviates manually aligning.
	Pool pool;

public:
	void init()
	{
		(void)pool_create(&pool, MAX_BLOCKS*FILE_BLOCK_SIZE, FILE_BLOCK_SIZE);
	}

	void shutdown()
	{
		(void)pool_destroy(&pool);
	}

	void* alloc(BlockId id)
	{
		if(blocks.size() == MAX_BLOCKS)
		{
			Block& b = blocks.front();
			// if this block is still locked, big trouble..
			// (someone forgot to free it and we can't reuse it)
			debug_assert(b.status != BS_PENDING);
			pool_free(&pool, b.mem);
			blocks.pop_front();
		}
		void* mem = pool_alloc(&pool, FILE_BLOCK_SIZE);	// can't fail
		blocks.push_back(Block(id, mem));
		return mem;
	}

	void mark_completed(BlockId id)
	{
		for(BlockIt it = blocks.begin(); it != blocks.end(); ++it)
		{
			if(it->id == id)
				it->status = BS_COMPLETE;
		}
	}

	void* find(BlockId id)
	{
		// linear search is ok, since we only keep a few blocks.
		for(BlockIt it = blocks.begin(); it != blocks.end(); ++it)
		{
			if(it->status == BS_COMPLETE && it->id == id)
				return it->mem;
		}
		return 0;	// not found
	}

	void invalidate(const char* atom_fn)
	{
		for(BlockIt it = blocks.begin(); it != blocks.end(); ++it)
			if((const char*)(it->id >> 32) == atom_fn)
				it->status = BS_INVALID;
	}
};
static BlockMgr block_mgr;


// create an id for use with the cache that uniquely identifies
// the block from the file <atom_fn> starting at <ofs> (aligned).
BlockId block_cache_make_id(const char* atom_fn, const off_t ofs)
{
	cassert(sizeof(atom_fn) == 4);
	// format: filename atom | block number
	//         63         32   31         0
	//
	// <atom_fn> is guaranteed to be unique (see file_make_unique_fn_copy).
	//
	// block_num should always fit in 32 bits (assuming maximum file size
	// = 2^32 * FILE_BLOCK_SIZE ~= 2^48 -- plenty). we don't bother
	// checking this.

	const size_t block_num = ofs / FILE_BLOCK_SIZE;
	return u64_from_u32((u32)(uintptr_t)atom_fn, (u32)block_num);
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
	return block_mgr.find(id);
}


//-----------------------------------------------------------------------------

// remove all blocks loaded from the file <fn>. used when reloading the file.
LibError file_cache_invalidate(const char* P_fn)
{
	const char* atom_fn = file_make_unique_fn_copy(P_fn, 0);
	block_mgr.invalidate(atom_fn);

	return ERR_OK;
}


void file_cache_init()
{
	block_mgr.init();
	cache_allocator.init();
}


void file_cache_shutdown()
{
	extant_bufs.display_all_remaining();
	cache_allocator.shutdown();
	block_mgr.shutdown();
}
