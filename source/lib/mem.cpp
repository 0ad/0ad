// malloc layer for less fragmentation, alignment, and automatic release

#include <cstdlib>
#include <cassert>

#include <map>

#include "types.h"
#include "mem.h"
#include "res.h"
#include "misc.h"
#include "posix.h"


static void heap_free(MEM* m)
{
	free(m->org_p);
}


static void* heap_alloc(const size_t size, const int align, MEM* mem)
{
	u8* org_p = (u8*)malloc(size+align-1);
	u8* p = (u8*)round_up((long)org_p, align);

	mem->org_p = org_p;	
	return p;
}


//////////////////////////////////////////////////////////////////////////////


static u8* pool;
static size_t pool_pos;
static const size_t POOL_CAP = 64*MB;	// TODO: user editable


static void pool_free(MEM* m)
{
	// at end of pool? if so, 'free' it
	if(m->ofs + m->size == pool_pos)
		pool_pos -= m->size;
}


static void* pool_alloc(const size_t size, const uint align, MEM* mem)
{
	if(!pool)
	{
		pool = (u8*)mem_alloc(size, align, MEM_HEAP);
		if(!pool)
			return 0;
	}

	size_t ofs = round_up((long)pool+pool_pos, align) - (ulong)pool;
	if(ofs+size > POOL_CAP)
		return 0;

	void* p = (u8*)pool + ofs;

	mem->size = size;
	mem->ofs = ofs;

	pool_pos = ofs+size;
	return p;
}


//////////////////////////////////////////////////////////////////////////////


static void mmap_free(MEM* m)
{
	munmap(m->p, m->size);
}


static void* mmap_alloc(const size_t size, const int fd, MEM* mem)
{
	mem->p = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	mem->size = size;
	mem->fd = fd;

	return mem->p;
}


//////////////////////////////////////////////////////////////////////////////


static void mem_dtor(void* p)
{
	MEM* m = (MEM*)p;
	if(m->type == MEM_HEAP)
		heap_free(m);
	else if(m->type == MEM_POOL)
		pool_free(m);
	else if(m->type == MEM_MAPPED)
		mmap_free(m);
	else
		assert(0 && "mem_dtor: MEM.type invalid!");
}


int mem_free(void* p)
{
	if(!p)
		return 1;

	Handle h = h_find((u32)p, H_MEM, 0);
	if(h)
		return h_free(h, H_MEM);
	return -1;
}


int mem_free(Handle hm)
{
	return h_free(hm, H_MEM);
}


void* mem_alloc(size_t size, const uint align, const MemType type, const int fd, Handle* ph)
{
	assert(size != 0 && "mem_alloc: why is size = 0?");

	// bit of a hack: the allocators require space for bookkeeping,
	// but we can't allocate a handle until we know the key
	// (the pointer address), which is used to find the corresponding
	// handle when freeing memory.
	// we fill a temp MEM, and then copy it into the handle's user data space
	MEM mem;

	void* p;
	if(type == MEM_HEAP)
		p = heap_alloc(size, align, &mem);
	else if(type == MEM_POOL)
		p = pool_alloc(size, align, &mem);
	else if(type == MEM_MAPPED)
		p = mmap_alloc(size, fd, &mem);
	else
	{
		assert(0 && "mem_alloc: invalid type parameter");
		return 0;
	}

	if(!p)
		return 0;

	MEM* pmem;
	Handle h = h_alloc((u32)p, H_MEM, mem_dtor, (void**)&pmem);
	if(!h)	// failed to allocate a handle
	{
		mem_dtor(&mem);
		return 0;
	}
	*pmem = mem;	// copy our memory info into the handle's user data space

	// caller is asking for the handle
	// (freeing the memory via handle is faster than mem_free, because
	//  we wouldn't have to scan all handles looking for the pointer)
	if(ph)
		*ph = h;

	return p;
}