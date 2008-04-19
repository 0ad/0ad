/**
 * =========================================================================
 * File        : bucket.cpp
 * Project     : 0 A.D.
 * Description : bucket allocator
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "bucket.h"

#include "lib/bits.h"
#include "mem_util.h"


// power-of-2 isn't required; value is arbitrary.
const size_t bucketSize = 4000;


LibError bucket_create(Bucket* b, size_t el_size)
{
	b->freelist = 0;
	b->el_size = mem_RoundUpToAlignment(el_size);

	// note: allocating here avoids the is-this-the-first-time check
	// in bucket_alloc, which speeds things up.
	b->bucket = (u8*)malloc(bucketSize);
	if(!b->bucket)
	{
		// cause next bucket_alloc to retry the allocation
		b->pos = bucketSize;
		b->num_buckets = 0;
		WARN_RETURN(ERR::NO_MEM);
	}

	*(u8**)b->bucket = 0;	// terminate list
	b->pos = mem_RoundUpToAlignment(sizeof(u8*));
	b->num_buckets = 1;
	return INFO::OK;
}


void bucket_destroy(Bucket* b)
{
	while(b->bucket)
	{
		u8* prev_bucket = *(u8**)b->bucket;
		free(b->bucket);
		b->bucket = prev_bucket;
		b->num_buckets--;
	}

	debug_assert(b->num_buckets == 0);

	// poison pill: cause subsequent alloc and free to fail
	b->freelist = 0;
	b->el_size = bucketSize;
}


void* bucket_alloc(Bucket* b, size_t size)
{
	size_t el_size = b->el_size? b->el_size : mem_RoundUpToAlignment(size);
	// must fit in a bucket
	debug_assert(el_size <= bucketSize-sizeof(u8*));

	// try to satisfy alloc from freelist
	void* el = mem_freelist_Detach(b->freelist);
	if(el)
		return el;

	// if there's not enough space left, close current bucket and
	// allocate another.
	if(b->pos+el_size > bucketSize)
	{
		u8* bucket = (u8*)malloc(bucketSize);
		if(!bucket)
			return 0;
		*(u8**)bucket = b->bucket;
		b->bucket = bucket;
		// skip bucket list field and align (note: malloc already
		// aligns to at least 8 bytes, so don't take b->bucket into account)
		b->pos = mem_RoundUpToAlignment(sizeof(u8*));;
		b->num_buckets++;
	}

	void* ret = b->bucket+b->pos;
	b->pos += el_size;
	return ret;
}


void* bucket_fast_alloc(Bucket* b)
{
	// try to satisfy alloc from freelist
	void* el = mem_freelist_Detach(b->freelist);
	if(el)
		return el;

	// if there's not enough space left, close current bucket and
	// allocate another.
	if(b->pos+b->el_size > bucketSize)
	{
		u8* bucket = (u8*)malloc(bucketSize);
		*(u8**)bucket = b->bucket;
		b->bucket = bucket;
		// skip bucket list field (alignment is only pointer-size)
		b->pos = sizeof(u8*);
		b->num_buckets++;
	}

	void* ret = b->bucket+b->pos;
	b->pos += b->el_size;
	return ret;
}


void bucket_free(Bucket* b, void* el)
{
	if(b->el_size == 0)
	{
		DEBUG_WARN_ERR(ERR::LOGIC);	// cannot free variable-size items
		return;
	}

	mem_freelist_AddToFront(b->freelist, el);

	// note: checking if <el> was actually allocated from <b> is difficult:
	// it may not be in the currently open bucket, so we'd have to
	// iterate over the list - too much work.
}
