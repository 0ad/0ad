// suballocators
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef ALLOCATORS_H__
#define ALLOCATORS_H__

#include "lib/types.h"
#include "lib/posix.h"	// PROT_* constants for da_set_prot


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
extern int da_alloc(DynArray* da, size_t max_size);

// free all memory (address space + physical) that constitutes the
// given array. use-after-free is impossible because the memory is
// marked not-present via MMU. also zeroes the contents of <da>.
extern int da_free(DynArray* da);

// expand or shrink the array: changes the amount of currently committed
// (i.e. usable) memory pages. pages are added/removed until
// new_size (rounded up to the next page size multiple) is met.
extern int da_set_size(DynArray* da, size_t new_size);

// change access rights of the array memory; used to implement
// write-protection. affects the currently committed pages as well as
// all subsequently added pages.
// prot can be a combination of the PROT_* values used with mprotect.
extern int da_set_prot(DynArray* da, int prot);

// "wrap" (i.e. store information about) the given buffer in a
// DynArray object, preparing it for use with da_read or da_append.
// da_free should be called when the DynArray is no longer needed,
// even though it doesn't free this memory (but does zero the DynArray).
extern int da_wrap_fixed(DynArray* da, u8* p, size_t size);

// "read" from array, i.e. copy into the given buffer.
// starts at offset DynArray.pos and advances this.
extern int da_read(DynArray* da, void* data_dst, size_t size);

// "write" to array, i.e. copy from the given buffer.
// starts at offset DynArray.pos and advances this.
extern int da_append(DynArray* da, const void* data_src, size_t size);



//
// pool allocator
//

// design parameters:
// - O(1) alloc and free;
// - fixed-size blocks;
// - doesn't preallocate the entire pool;
// - returns sequential addresses.

// opaque! do not read/write any fields!
struct Pool
{
	DynArray da;
	size_t el_size;

	// all bytes in da up to this mark are in circulation or freelist.
	size_t pos;

	// pointer to freelist (opaque); see freelist_*.
	void* freelist;
};

// ready <p> for use. pool_alloc will return chunks of memory that
// are exactly <el_size> bytes. <max_size> is the upper limit [bytes] on
// pool size (this is how much address space is reserved).
//
// note: el_size must at least be enough for a pointer (due to freelist
// implementation) but not exceed the expand-by amount.
extern int pool_create(Pool* p, size_t max_size, size_t el_size);

// free all memory that ensued from <p>. all elements are made unusable
// (it doesn't matter if they were "allocated" or in freelist or unused);
// future alloc and free calls on this pool will fail.
extern int pool_destroy(Pool* p);

// indicate whether <el> was allocated from the given pool.
// this is useful for callers that use several types of allocators.
extern bool pool_contains(Pool* p, void* el);

// return an entry from the pool, or 0 if it cannot be expanded as necessary.
// exhausts the freelist before returning new entries to improve locality.
extern void* pool_alloc(Pool* p);

// make <el> available for reuse in the given pool.
extern void pool_free(Pool* p, void* el);


//
// bucket allocator
//

// design goals:
// - variable-size allocations;
// - no reuse of allocations, can only free all at once;
// - no init necessary;
// - no fixed limit.

// opaque! do not read/write any fields!
struct Bucket
{
	// currently open bucket. must be initialized to 0.
	u8* bucket;

	// offset of free space at end of current bucket (i.e. # bytes in use).
	// must be initialized to 0.
	size_t pos;

	// records # buckets allocated; used to check if the list of them
	// isn't corrupted. must be initialized to 0.
	uint num_buckets;
};


// allocate <size> bytes of memory from the given Bucket object.
// <b> must initially be zeroed (e.g. by defining it as static data).
extern void* bucket_alloc(Bucket* b, size_t size);

// free all allocations that ensued from the given Bucket.
extern void bucket_free_all(Bucket* b);


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
// overrun protection
//

// this class wraps an arbitrary object in DynArray memory and can detect
// inadvertent writes to it. this is useful for tracking down memory overruns.
//
// the basic idea is to require users to request access to the object and
// notify us when done; memory access permission is temporarily granted.
// (similar in principle to Software Transaction Memory).
//
// since this is quite slow, the protection is disabled unless
// CONFIG_OVERRUN_PROTECTION == 1; this avoids having to remove the
// wrapper code in release builds and re-write when looking for overruns.
//
// example usage:
// OverrunProtector<your_class> your_class_wrapper;
// ..
// your_class* yc = your_class_wrapper.get();
// if(!yc) abort();	// not enough memory to allocate a your_class instance
// // access/write to <yc>
// your_class_wrapper.lock();	// disallow further access
// ..
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
		const size_t size = 4096;
		cassert(sizeof(T) <= size);
		if(da_alloc(&da, size) < 0)
			goto fail;
		if(da_set_size(&da, size) < 0)
			goto fail;

#include "nommgr.h"
		cached_ptr = new(da.base) T();
#include "mmgr.h"
		lock();
		return;	// success

fail:
		debug_warn("OverrunProtector mem alloc failed");
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
