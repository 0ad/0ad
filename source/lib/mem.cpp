// malloc layer for less fragmentation, alignment, and automatic release

#include <cstdlib>

#include <map>

#include "types.h"
#include "mem.h"
#include "res.h"
#include "misc.h"
#include "posix.h"


struct MEM
{
	uint type;

	// MEM_HEAP only
	void* org_p;

	// MEM_POOL only
	size_t ofs;
	size_t size;
};



static void heap_dtor(void* p)
{
	MEM* mem = (MEM*)p;
	free(mem->org_p);
}


static void* heap_alloc(const size_t size, const int align, MEM& mem)
{
	u8* org_p = (u8*)malloc(size+align-1);
	u8* p = (u8*)round_up((long)org_p, align);

	mem.org_p = org_p;	
	return p;
}



static u8* pool;
static size_t pool_pos;
static const size_t POOL_CAP = 64*MB;	// TODO: user editable


static void pool_dtor(void* p)
{
	MEM* mem = (MEM*)p;

	// at end of pool? if so, 'free' it
	if(mem->ofs + mem->size == pool_pos)
		pool_pos -= mem->size;
}


static void* pool_alloc(const size_t size, const uint align, MEM& mem)
{
	if(!pool)
	{
		pool = (u8*)mem_alloc(size, MEM_HEAP, align);
		if(!pool)
			return 0;
	}

	size_t ofs = round_up((long)pool+pool_pos, align) - (ulong)pool;
	if(ofs+size > POOL_CAP)
		return 0;

	void* p = (u8*)pool + ofs;

	mem.size = size;
	mem.ofs = ofs;

	pool_pos = ofs+size;
	return p;
}



static void mem_dtor(void* p)
{
	MEM* mem = (MEM*)p;
	if(mem->type == MEM_HEAP)
		heap_dtor(p);
	else if(mem->type == MEM_POOL)
		pool_dtor(p);
}


void* mem_alloc(size_t size, const MemType type, const uint align, Handle* ph)
{
	if(size == 0)
		size = 1;

	HDATA* hd;
	Handle h = h_alloc(0, H_MEM, mem_dtor, &hd);
	if(!h)
		return 0;

	MEM& mem = (MEM&)hd->user;

	void* p;
	if(type == MEM_HEAP)
		p = heap_alloc(size, align, mem);
	else if(type == MEM_POOL)
		p = pool_alloc(size, align, mem);
	else
	{
		h_free(h, H_MEM);
		return 0;
	}

	if(ph)
		*ph = h;

	return p;
}


int mem_free(void* p)
{
	if(!p)
		return 1;

	HDATA* hd;
	Handle h = h_find(p, H_MEM, &hd);
	if(h)
		return h_free(h, H_MEM);
	return -1;
}