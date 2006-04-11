/**
 * =========================================================================
 * File        : allocators.h
 * Project     : 0 A.D.
 * Description : memory suballocators.
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

#ifndef ALLOCATORS_H__
#define ALLOCATORS_H__

#include "lib/types.h"
#include "lib/posix.h"	// PROT_* constants for da_set_prot

//
// page aligned allocator
//

// allocates memory aligned to the system page size.
//
// this is useful for file_buf_alloc, which uses this allocator to
// get sector-aligned (hopefully; see file_sector_size) IO buffers.
//
// note that this allocator is stateless and very litte error checking
// can be performed.

// returns at least unaligned_size bytes of page-aligned memory.
// it defaults to read/writable; you can mprotect it if desired.
extern void* page_aligned_alloc(size_t unaligned_size);

// free a previously allocated region. must be passed the exact values
// passed to/returned from page_aligned_malloc.
extern void page_aligned_free(void* p, size_t unaligned_size);


//
// dynamic (expandable) array
//

// provides a memory range that can be expanded but doesn't waste
// physical memory or relocate itself. building block for other allocators.

struct DynArray
{
	u8* base;
	size_t max_size_pa;	// reserved
	size_t cur_size;	// committed

	int prot;	// applied to newly committed pages

	size_t pos;
};


// ready the DynArray object for use. preallocates max_size bytes
// (rounded up to the next page size multiple) of address space for the
// array; it can never grow beyond this.
// no virtual memory is actually committed until calls to da_set_size.
extern LibError da_alloc(DynArray* da, size_t max_size);

// free all memory (address space + physical) that constitutes the
// given array. use-after-free is impossible because the memory is
// marked not-present via MMU. also zeroes the contents of <da>.
extern LibError da_free(DynArray* da);

// expand or shrink the array: changes the amount of currently committed
// (i.e. usable) memory pages. pages are added/removed until
// new_size (rounded up to the next page size multiple) is met.
extern LibError da_set_size(DynArray* da, size_t new_size);

// make sure at least <size> bytes starting at <pos> are committed and
// ready for use.
extern LibError da_reserve(DynArray* da, size_t size);

// change access rights of the array memory; used to implement
// write-protection. affects the currently committed pages as well as
// all subsequently added pages.
// prot can be a combination of the PROT_* values used with mprotect.
extern LibError da_set_prot(DynArray* da, int prot);

// "wrap" (i.e. store information about) the given buffer in a
// DynArray object, preparing it for use with da_read or da_append.
// da_free should be called when the DynArray is no longer needed,
// even though it doesn't free this memory (but does zero the DynArray).
extern LibError da_wrap_fixed(DynArray* da, u8* p, size_t size);

// "read" from array, i.e. copy into the given buffer.
// starts at offset DynArray.pos and advances this.
extern LibError da_read(DynArray* da, void* data_dst, size_t size);

// "write" to array, i.e. copy from the given buffer.
// starts at offset DynArray.pos and advances this.
extern LibError da_append(DynArray* da, const void* data_src, size_t size);



//
// pool allocator
//

// design parameters:
// - O(1) alloc and free;
// - fixed- XOR variable-sized blocks;
// - doesn't preallocate the entire pool;
// - returns sequential addresses.

// opaque! do not read/write any fields!
struct Pool
{
	DynArray da;

	// size of elements. = 0 if pool set up for variable-sized
	// elements, otherwise rounded up to pool alignment.
	size_t el_size;

	// pointer to freelist (opaque); see freelist_*.
	// never used (remains 0) if elements are of variable size.
	void* freelist;
};

// pass as pool_create's <el_size> param to indicate variable-sized allocs
// are required (see below).
const size_t POOL_VARIABLE_ALLOCS = ~0u;

// ready <p> for use. <max_size> is the upper limit [bytes] on
// pool size (this is how much address space is reserved).
//
// <el_size> can be 0 to allow variable-sized allocations
//  (which cannot be freed individually);
// otherwise, it specifies the number of bytes that will be
// returned by pool_alloc (whose size parameter is then ignored).
extern LibError pool_create(Pool* p, size_t max_size, size_t el_size);

// free all memory that ensued from <p>. all elements are made unusable
// (it doesn't matter if they were "allocated" or in freelist or unused);
// future alloc and free calls on this pool will fail.
extern LibError pool_destroy(Pool* p);

// indicate whether <el> was allocated from the given pool.
// this is useful for callers that use several types of allocators.
extern bool pool_contains(Pool* p, void* el);

// return an entry from the pool, or 0 if it would have to be expanded and
// there isn't enough memory to do so.
// exhausts the freelist before returning new entries to improve locality.
//
// if the pool was set up with fixed-size elements, <size> is ignored;
// otherwise, <size> bytes are allocated.
extern void* pool_alloc(Pool* p, size_t size);

// make <el> available for reuse in the given Pool.
//
// this is not allowed if created for variable-size elements.
// rationale: avoids having to pass el_size here and compare with size when
// allocating; also prevents fragmentation and leaking memory.
extern void pool_free(Pool* p, void* el);

// "free" all allocations that ensued from the given Pool.
// this resets it as if freshly pool_create-d, but doesn't release the
// underlying memory.
extern void pool_free_all(Pool* p);


//
// bucket allocator
//

// design goals:
// - fixed- XOR variable-sized blocks;
// - allow freeing individual blocks if they are all fixed-size;
// - never relocates;
// - no fixed limit.

// note: this type of allocator is called "region-based" in the literature.
// see "Reconsidering Custom Memory Allocation" (Berger, Zorn, McKinley).
// if individual variable-size elements must be freeable, consider "reaps":
// basically a combination of region and heap, where frees go to the heap and
// allocs exhaust that memory first and otherwise use the region.

// opaque! do not read/write any fields!
struct Bucket
{
	// currently open bucket.
	u8* bucket;

	// offset of free space at end of current bucket (i.e. # bytes in use).
	size_t pos;

	void* freelist;

	size_t el_size : 16;

	// records # buckets allocated; verifies the list of buckets is correct.
	uint num_buckets : 16;
};


// ready <b> for use.
//
// <el_size> can be 0 to allow variable-sized allocations
//  (which cannot be freed individually);
// otherwise, it specifies the number of bytes that will be
// returned by bucket_alloc (whose size parameter is then ignored).
extern LibError bucket_create(Bucket* b, size_t el_size);

// free all memory that ensued from <b>.
// future alloc and free calls on this Bucket will fail.
extern void bucket_destroy(Bucket* b);

// return an entry from the bucket, or 0 if another would have to be
// allocated and there isn't enough memory to do so.
// exhausts the freelist before returning new entries to improve locality.
//
// if the bucket was set up with fixed-size elements, <size> is ignored;
// otherwise, <size> bytes are allocated.
extern void* bucket_alloc(Bucket* b, size_t size);

// make <el> available for reuse in <b>.
//
// this is not allowed if created for variable-size elements.
// rationale: avoids having to pass el_size here and compare with size when
// allocating; also prevents fragmentation and leaking memory.
extern void bucket_free(Bucket* b, void* el);


//
// matrix allocator
//

// takes care of the dirty work of allocating 2D matrices:
// - aligns data
// - only allocates one memory block, which is more efficient than
//   malloc/new for each row.

// allocate a 2D cols x rows matrix of <el_size> byte cells.
// this must be freed via matrix_free. returns 0 if out of memory.
//
// the returned pointer should be cast to the target type (e.g. int**) and
// can then be accessed by matrix[col][row].
//
extern void** matrix_alloc(uint cols, uint rows, size_t el_size);

// free the given matrix (allocated by matrix_alloc). no-op if matrix == 0.
// callers will likely want to pass variables of a different type
// (e.g. int**); they must be cast to void**.
extern void matrix_free(void** matrix);


//
// allocator optimized for single instances
//

// intended for applications that frequently alloc/free a single
// fixed-size object. caller provides static storage and an in-use flag;
// we use that memory if available and otherwise fall back to the heap.
// if the application only has one object in use at a time, malloc is
// avoided; this is faster and avoids heap fragmentation.
//
// thread-safe.

extern void* single_calloc(void* storage, volatile uintptr_t* in_use_flag, size_t size);

extern void single_free(void* storage, volatile uintptr_t* in_use_flag, void* p);

// C++ wrapper
#ifdef __cplusplus

// T must be POD (Plain Old Data) because it is memset to 0!
template<class T> class SingleAllocator
{
	T storage;
	volatile uintptr_t is_in_use;

public:
	SingleAllocator()
	{
		is_in_use = 0;
	}

	void* alloc()
	{
		return single_calloc(&storage, &is_in_use, sizeof(storage));
	}

	void release(void* p)
	{
		single_free(&storage, &is_in_use, p);
	}
};

#endif	// #ifdef __cplusplus


//
// overrun protection
//

/*
OverrunProtector wraps an arbitrary object in DynArray memory and can detect
inadvertent writes to it. this is useful for tracking down memory overruns.

the basic idea is to require users to request access to the object and
notify us when done; memory access permission is temporarily granted.
(similar in principle to Software Transaction Memory).

since this is quite slow, the protection is disabled unless
CONFIG_OVERRUN_PROTECTION == 1; this avoids having to remove the
wrapper code in release builds and re-write when looking for overruns.

example usage:
OverrunProtector<your_class> your_class_wrapper;
..
your_class* yc = your_class_wrapper.get();	// unlock, make ready for use
if(!yc)			// your_class_wrapper's one-time alloc of a your_class-
	abort();	// instance had failed - can't continue.
doSomethingWith(yc);	// read/write access
your_class_wrapper.lock();	// disallow further access until next .get()
..
*/

template<class T> class OverrunProtector
{
	DynArray da;
	T* cached_ptr;
	uintptr_t initialized;

public:
	OverrunProtector()
	{
		memset(&da, 0, sizeof(da));
		cached_ptr = 0;
		initialized = 0;
	}

	~OverrunProtector()
	{
		shutdown();
	}

	void lock()
	{
#if CONFIG_OVERRUN_PROTECTION
		da_set_prot(&da, PROT_NONE);
#endif
	}

private:
	void unlock()
	{
#if CONFIG_OVERRUN_PROTECTION
		da_set_prot(&da, PROT_READ|PROT_WRITE);
#endif
	}

	void init()
	{
		if(da_alloc(&da, sizeof(T)) < 0)
		{
fail:
			WARN_ERR(ERR_NO_MEM);
			return;
		}
		if(da_set_size(&da, sizeof(T)) < 0)
			goto fail;

#include "nommgr.h"
		cached_ptr = new(da.base) T();
#include "mmgr.h"
		lock();
	}

	void shutdown()
	{
		if(!CAS(&initialized, 1, 2))
			return;	// never initialized or already shut down - abort
		unlock();
		cached_ptr->~T();	// call dtor (since we used placement new)
		cached_ptr = 0;
		(void)da_free(&da);
	}

public:
	T* get()
	{
		// this could theoretically be done in the ctor, but we try to
		// minimize non-trivial code at NLSO ctor time
		// (avoids init order problems).
		if(CAS(&initialized, 0, 1))
			init();
		debug_assert(initialized != 2 && "OverrunProtector: used after dtor called:");
		unlock();
		return cached_ptr;
	}
};

#endif	// #ifndef ALLOCATORS_H__
