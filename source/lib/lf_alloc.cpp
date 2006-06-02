/**
 * =========================================================================
 * File        : lf_alloc.cpp
 * Project     : 0 A.D.
 * Description : lock-free memory allocation.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#if 0

#include <algorithm>
#include <limits.h>

#include "lib.h"
#include "posix.h"
#include "lib/sysdep/cpu.h"
#include "lockfree.h"
#include "timer.h"


// superblock descriptor structure
// one machine word
struct Anchor
{
	uint avail : 10;
	uint count : 10;
	uint tag   : 10;
	uint state :  2;

	// convert to uintptr_t for CAS
	operator uintptr_t() const
	{
		return *(uintptr_t*)this;
	}
};

cassert(sizeof(Anchor) == sizeof(uintptr_t));

enum State
{
	ACTIVE  = 0,
	FULL    = 1,
	PARTIAL = 2,
	EMPTY   = 3
};


/*/**/typedef void* DescList;

struct SizeClass
{
	DescList partial;	// initially empty
	size_t sz;	// block size
	size_t sb_size;	// superblock's size
};

struct Descriptor;

static const uint PTR_BITS = sizeof(void*) * CHAR_BIT;

struct Active
{
	uint pdesc : PTR_BITS-6;
	uint credits : 6;

	Active()
	{
	}

	// convert to uintptr_t for CAS
	operator uintptr_t() const
	{
		return *(uintptr_t*)this;
	}


	//
	// allow Active to be used as Descriptor*
	//

	Active& operator=(Descriptor* desc)
	{
		*(Descriptor**)this = desc;
		debug_assert(credits == 0);	// make sure ptr is aligned
		return *this;
	}

	Active(Descriptor* desc)
	{
		*this = desc;
	}

	// disambiguate (could otherwise be either uintptr_t or Descriptor*)
	bool operator!() const
	{
		return (uintptr_t)*this != 0;
	}

	operator Descriptor*() const
	{
		return *(Descriptor**)this;
	}

};

static const uint MAX_CREDITS = 64;	// = 2 ** num_credit_bits

struct ProcHeap
{
	Active active;			// initially 0; points to Descriptor
	Descriptor* partial;	// initially 0
	SizeClass* sc;	// parent
};

// POD; must be MAX_CREDITS-aligned!
struct Descriptor
{
	Anchor anchor;
	Descriptor* next;
	u8* sb;	// superblock
	ProcHeap* heap;	// -> owner procheap
	size_t sz;	// block size
	uint maxcount;	// superblock size/sz
};






static u8* AllocNewSB(size_t sb_size)
{
	return 0;
}

static void FreeSB(u8* sb)
{
}


static Descriptor* DescAvail = 0;

static const size_t DESCSBSIZE = 128;

static Descriptor* DescAlloc()
{
	Descriptor* desc;
	for(;;)
	{
		desc = DescAvail;
		if(desc)
		{
			Descriptor* next = desc->next;
			if(CAS(&DescAvail, desc, next))
				break;
		}
		else
		{
			desc = (Descriptor*)AllocNewSB(DESCSBSIZE);
			// organize descriptors in a linked list
			mfence();
			if(CAS(&DescAvail, 0, desc->next))
				break;
			FreeSB((u8*)desc);
		}
	}
	return desc;
}

static void DescRetire(Descriptor* desc)
{
	Descriptor* old_head;
	do
	{
		old_head = DescAvail;
		desc->next = old_head;
		mfence();
	}
	while(!CAS(&DescAvail, old_head, desc));
}

static Descriptor* ListGetPartial(SizeClass* sc)
{
	return 0;
}

static void ListPutPartial(Descriptor* desc)
{
}

static void ListRemoveEmptyDesc(SizeClass* sc)
{
}

static ProcHeap* find_heap(SizeClass* sc)
{
	return 0;
}


static Descriptor* HeapGetPartial(ProcHeap* heap)
{
	Descriptor* desc;
	do
	{
		desc = heap->partial;
		if(!desc)
			return ListGetPartial(heap->sc);
	}
	while(!CAS(&heap->partial, desc, 0));
	return desc;
}


static void HeapPutPartial(Descriptor* desc)
{
	Descriptor* prev;
	do
	prev = desc->heap->partial;
	while(!CAS(&desc->heap->partial, prev, desc));
	if(prev)
		ListPutPartial(prev);
}


static void UpdateActive(ProcHeap* heap, Descriptor* desc, uint more_credits)
{
	Active new_active = desc;
	new_active.credits = more_credits-1;
	if(CAS(&heap->active, 0, new_active))
		return;

	// someone installed another active sb
	// return credits to sb and make it partial
	Anchor old_anchor, new_anchor;
	do
	{
		new_anchor = old_anchor = desc->anchor;
		new_anchor.count += more_credits;
		new_anchor.state = PARTIAL;
	}
	while(!CAS(&desc->anchor, old_anchor, new_anchor));
	HeapPutPartial(desc);
}



static void RemoveEmptyDesc(ProcHeap* heap, Descriptor* desc)
{
	if(CAS(&heap->partial, desc, 0))
		DescRetire(desc);
	else
		ListRemoveEmptyDesc(heap->sc);
}


static void* MallocFromActive(ProcHeap* heap)
{
	// reserve block
	Active old_active, new_active;
	do
	{
		new_active = old_active = heap->active;
		// no active superblock - will try Partial and then NewSB
		if(!old_active)
			return 0;
		// none left - mark as no longer active
		if(old_active.credits == 0)
			new_active = 0;
		// expected case - reserve
		else
			new_active.credits--;
	}
	while(!CAS(&heap->active, old_active, new_active));

	u8* p;

	// pop block
	Anchor old_anchor, new_anchor;
	Descriptor* desc = old_active;
	uint more_credits;
	do
	{
		new_anchor = old_anchor = desc->anchor;
		p = desc->sb + old_anchor.avail*desc->sz;
		new_anchor.avail = *(uint*)p;
		new_anchor.tag++;
		if(old_active.credits == 0)
		{
			// state must be ACTIVE
			if(old_anchor.count == 0)
				new_anchor.state = FULL;
			else
			{
				more_credits = MIN(old_anchor.count, MAX_CREDITS);
				new_anchor.count -= more_credits;
			}
		}
	}
	while(!CAS(&desc->anchor, old_anchor, new_anchor));
	if(old_active.credits == 0 && old_anchor.count > 0)
		UpdateActive(heap, desc, more_credits);

	*(Descriptor**)p = desc;
	return p+sizeof(void*);

}


static void* MallocFromPartial(ProcHeap* heap)
{
retry:
	Descriptor* desc = HeapGetPartial(heap);
	if(!desc)
		return 0;
	desc->heap = heap;

	// reserve blocks
	uint more_credits;
	Anchor old_anchor, new_anchor;
	do
	{
		new_anchor = old_anchor = desc->anchor;
		if(old_anchor.state == EMPTY)
		{
			DescRetire(desc);
			goto retry;
		}
		// old_anchor state must be PARTIAL
		// old_anchor count must be > 0
		more_credits = MIN(old_anchor.count-1, MAX_CREDITS);
		new_anchor.count -= more_credits+1;
		new_anchor.state = (more_credits > 0)? ACTIVE : FULL;
	}
	while(!CAS(&desc->anchor, old_anchor, new_anchor));

	u8* p;

	// pop reserved block
	do
	{
		new_anchor = old_anchor = desc->anchor;
		p = desc->sb + old_anchor.avail*desc->sz;
		new_anchor.avail = *(uint*)p;
		new_anchor.tag++;
	}
	while(!CAS(&desc->anchor, old_anchor, new_anchor));

	if(more_credits > 0)
		UpdateActive(heap, desc, more_credits);

	*(Descriptor**)p = desc;
	return p+sizeof(void*);
}


static void* MallocFromNewSB(ProcHeap* heap)
{
	Descriptor* desc = DescAlloc();
	desc->sb = AllocNewSB(heap->sc->sb_size);

	//organize blocks in a linked list starting with index 0

	desc->heap = heap;
	desc->anchor.avail = 1;
	desc->sz = heap->sc->sz;
	desc->maxcount = (uint)(heap->sc->sb_size/desc->sz);
	Active new_active = (Active)desc;
	new_active.credits = MIN(desc->maxcount-1, MAX_CREDITS)-1;
	desc->anchor.count = (desc->maxcount-1)-(new_active.credits+1);
	desc->anchor.state = ACTIVE;
	mfence();
	if(!CAS(&heap->active, 0, new_active))
	{
		FreeSB(desc->sb);
		return 0;
	}

	u8* p = desc->sb;

	*(Descriptor**)p = desc;
	return p+sizeof(void*);
}


void* lf_malloc(size_t sz)
{
	void* p;

	// use sz and thread id to find heap
	ProcHeap* heap = find_heap(0);	// TODO: pass SizeClass
	// large block - allocate directly
	if(!heap)
	{
		p = malloc(sz);
		if(p)
			*(size_t*)p = sz|1;
		return p;
	}

retry:
	p = MallocFromActive(heap);
	if(p)
		return p;
	p = MallocFromPartial(heap);
	if(p)
		return p;
	p = MallocFromNewSB(heap);
	if(p)
		return p;
	goto retry;
}


void lf_free(void* p_)
{
	if(!p_)
		return;
	u8* p = (u8*)p_;

	// get block header
	p -= sizeof(void*);
	uintptr_t hdr = *(uintptr_t*)p;

	// large block - free directly
	if(hdr & 1)
	{
		free(p);
		return;
	}

	Descriptor* desc = (Descriptor*)hdr;
	u8* sb = desc->sb;
	Anchor old_anchor, new_anchor;
	ProcHeap* heap;
	do
	{
		new_anchor = old_anchor = desc->anchor;
		*(size_t*)p = old_anchor.avail;
		new_anchor.avail = (uint)((p-sb) / desc->sz);
		if(old_anchor.state == FULL)
			new_anchor.state = PARTIAL;
		if(old_anchor.count == desc->maxcount-1)
		{
			heap = desc->heap;
			serialize();
			new_anchor.state = EMPTY;
		}
		else
			new_anchor.count++;
		mfence();
	}
	while(!CAS(&desc->anchor, old_anchor, new_anchor));
	if(new_anchor.state == EMPTY)
	{
		FreeSB(sb);
		RemoveEmptyDesc(heap, desc);
	}
	else if(old_anchor.state == FULL)
		HeapPutPartial(desc);
}

/*
static const int MAX_POOLS = 8;

// split out of pools[] for more efficient lookup
static size_t pool_element_sizes[MAX_POOLS];

struct Pool
{
u8* bucket_pos;
u8* freelist;
}
pools[MAX_POOLS];

static const int num_pools = 0;


const size_t BUCKET_SIZE = 8*KiB;

static u8* bucket_pos;


// return the pool responsible for <size>, or 0 if not yet set up and
// there are already too many pools.
static Pool* responsible_pool(size_t size)
{
Pool* pool = pools;
for(int i = 0; i < MAX_POOLS; i++, pool++)
if(pool->element_size == size)
return pool;

// need to set up a new pool
// .. but there are too many
debug_assert(0 <= num_pools && num_pools <= MAX_POOLS);
if(num_pools >= MAX_POOLS)
{
debug_warn("increase MAX_POOLS");
return 0;
}

pool = &pools[num_pools++];
pool->element_size = size;
return pool;
}

void* sbh_alloc(size_t size)
{
// when this allocation is freed, there must be enough room for
// our freelist pointer. also ensures alignment.
size = round_up(size, 8);

// would overflow a bucket
if(size > BUCKET_SIZE-sizeof(u8*))
{
debug_warn("sbh_alloc: size doesn't fit in a bucket");
return 0;
}


//

//

}


TNode* node_alloc(size_t size)
{
// would overflow a bucket
if(size > BUCKET_SIZE-sizeof(u8*))
{
debug_warn("node_alloc: size doesn't fit in a bucket");
return 0;
}

size = round_up(size, 8);
// ensure alignment, since size includes a string
const uintptr_t addr = (uintptr_t)bucket_pos;
const size_t bytes_used = addr % BUCKET_SIZE;
// addr = 0 on first call (no bucket yet allocated)
// bytes_used == 0 if a node fit exactly into a bucket
if(addr == 0 || bytes_used == 0 || bytes_used+size > BUCKET_SIZE)
{
u8* const prev_bucket = (u8*)addr - bytes_used;
u8* bucket = (u8*)mem_alloc(BUCKET_SIZE, BUCKET_SIZE);
if(!bucket)
return 0;
*(u8**)bucket = prev_bucket;
bucket_pos = bucket+round_up(sizeof(u8*), 8);
}

TNode* node = (TNode*)bucket_pos;
bucket_pos = (u8*)node+size;
return node;
}


static void node_free_all()
{
const uintptr_t addr = (uintptr_t)bucket_pos;
u8* bucket = bucket_pos - (addr % BUCKET_SIZE);

// covers bucket_pos == 0 case
while(bucket)
{
u8* prev_bucket = *(u8**)bucket;
mem_free(bucket);
bucket = prev_bucket;
}
}
*/
#endif
