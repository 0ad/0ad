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
	void* org_p;	// MEM_HEAP only
	size_t size;	// MEM_POOL only
};



static void heap_dtor(HDATA* hd)
{
	MEM* mem = (MEM*)hd->internal;
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


static void pool_dtor(HDATA* hd)
{
	MEM* mem = (MEM*)hd->internal;

	ssize_t pool_ofs = (u8*)hd->p - (u8*)pool;
	// at end of pool? if so, 'free' it
	if(pool_ofs + mem->size == pool_pos)
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

	pool_pos = ofs+size;
	return p;
}



static void mem_dtor(HDATA* hd)
{
	MEM* mem = (MEM*)hd->internal;
	if(mem->type == MEM_HEAP)
		heap_dtor(hd);
	else if(mem->type == MEM_POOL)
		pool_dtor(hd);
}


void* mem_alloc(size_t size, const MemType type, const uint align, Handle* ph)
{
	if(size == 0)
		size = 1;

	HDATA* hd;
	Handle h = h_alloc(0, RES_MEM, mem_dtor, &hd);
	if(!h)
		return 0;

	MEM& mem = (MEM&)hd->internal;

	void* p;
	if(type == MEM_HEAP)
		p = heap_alloc(size, align, mem);
	else if(type == MEM_POOL)
		p = pool_alloc(size, align, mem);
	else
	{
		h_free(h, RES_MEM);
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
	Handle h = h_find(p, RES_MEM, &hd);
	if(h)
		return h_free(h, RES_MEM);
	return -1;
}