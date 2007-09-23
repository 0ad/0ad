/**
 * =========================================================================
 * File        : allocators.h
 * Project     : 0 A.D.
 * Description : memory suballocators.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_ALLOCATORS
#define INCLUDED_ALLOCATORS

#include <map>

#include "lib/posix/posix_mman.h"	// PROT_*
#include "lib/sysdep/cpu.h"	// cpu_CAS


//
// page aligned allocator
//

/**
 * allocate memory aligned to the system page size.
 *
 * this is useful for file_buf_alloc, which uses this allocator to
 * get sector-aligned (hopefully; see file_sector_size) IO buffers.
 *
 * note that this allocator is stateless and very litte error checking
 * can be performed.
 *
 * the memory is initially writable and you can use mprotect to set other
 * access permissions if desired.
 *
 * @param unaligned_size minimum size [bytes] to allocate.
 * @return page-aligned and -padded memory or 0 on error / out of memory.
 **/
extern void* page_aligned_alloc(size_t unaligned_size);

/**
 * free a previously allocated page-aligned region.
 *
 * @param p exact value returned from page_aligned_alloc
 * @param size exact value passed to page_aligned_alloc
 **/
extern void page_aligned_free(void* p, size_t unaligned_size);


//
// dynamic (expandable) array
//

/**
 * provides a memory range that can be expanded but doesn't waste
 * physical memory or relocate itself.
 *
 * works by preallocating address space and committing as needed.
 * used as a building block for other allocators.
 **/
struct DynArray
{
	u8* base;
	size_t max_size_pa;	 /// reserved
	size_t cur_size;	 /// committed
	size_t cur_size_pa;

	/**
	 * mprotect flags applied to newly committed pages
	 **/
	int prot;

	size_t pos;
};


/**
 * ready the DynArray object for use.
 *
 * no virtual memory is actually committed until calls to da_set_size.
 *
 * @param da DynArray.
 * @param max_size size [bytes] of address space to reserve (*);
 * the DynArray can never expand beyond this.
 * (* rounded up to next page size multiple)
 * @return LibError.
 **/
extern LibError da_alloc(DynArray* da, size_t max_size);

/**
 * free all memory (address space + physical) that constitutes the
 * given array.
 *
 * use-after-free is impossible because the memory is unmapped.
 *
 * @param DynArray* da; zeroed afterwards.
 * @return LibError
 **/
extern LibError da_free(DynArray* da);

/**
 * expand or shrink the array: changes the amount of currently committed
 * (i.e. usable) memory pages.
 *
 * @param da DynArray.
 * @param new_size target size (rounded up to next page multiple).
 * pages are added/removed until this is met.
 * @return LibError.
 **/
extern LibError da_set_size(DynArray* da, size_t new_size);

/**
 * Make sure at least <size> bytes starting at da->pos are committed and
 * ready for use.
 *
 * @param DynArray*
 * @param size Minimum amount to guarantee [bytes]
 * @return LibError
 **/
extern LibError da_reserve(DynArray* da, size_t size);

/**
 * change access rights of the array memory.
 *
 * used to implement write-protection. affects the currently committed
 * pages as well as all subsequently added pages.
 *
 * @param da DynArray.
 * @param prot a combination of the PROT_* values used with mprotect.
 * @return LibError.
 **/
extern LibError da_set_prot(DynArray* da, int prot);

/**
 * "wrap" (i.e. store information about) the given buffer in a DynArray.
 *
 * this is used to allow calling da_read or da_append on normal buffers.
 * da_free should be called when the DynArray is no longer needed,
 * even though it doesn't free this memory (but does zero the DynArray).
 *
 * @param da DynArray. Note: any future operations on it that would
 * change the underlying memory (e.g. da_set_size) will fail.
 * @param p target memory (no alignment/padding requirements)
 * @param size maximum size (no alignment requirements)
 * @return LibError.
 **/
extern LibError da_wrap_fixed(DynArray* da, u8* p, size_t size);

/**
 * "read" from array, i.e. copy into the given buffer.
 *
 * starts at offset DynArray.pos and advances this.
 *
 * @param da DynArray.
 * @param data_dst destination memory
 * @param size [bytes] to copy
 * @return LibError.
 **/
extern LibError da_read(DynArray* da, void* data_dst, size_t size);

/**
 * "write" to array, i.e. copy from the given buffer.
 *
 * starts at offset DynArray.pos and advances this.
 *
 * @param da DynArray.
 * @param data_src source memory
 * @param size [bytes] to copy
 * @return LibError.
 **/
extern LibError da_append(DynArray* da, const void* data_src, size_t size);


//
// pool allocator
//

/**
 * allocator design parameters:
 * - O(1) alloc and free;
 * - either fixed- or variable-sized blocks;
 * - doesn't preallocate the entire pool;
 * - returns sequential addresses.
 *
 * opaque! do not read/write any fields!
 **/
struct Pool
{
	DynArray da;

	/**
	 * size of elements. = 0 if pool set up for variable-sized
	 * elements, otherwise rounded up to pool alignment.
	 **/
	size_t el_size;

	/**
	 * pointer to freelist (opaque); see freelist_*.
	 * never used (remains 0) if elements are of variable size.
	 **/
	void* freelist;
};

/**
 * pass as pool_create's <el_size> param to indicate variable-sized allocs
 * are required (see below).
 **/
const size_t POOL_VARIABLE_ALLOCS = ~0u;

/**
 * Ready Pool for use.
 *
 * @param Pool*
 * @param max_size Max size [bytes] of the Pool; this much
 * (rounded up to next page multiple) virtual address space is reserved.
 * no virtual memory is actually committed until calls to pool_alloc.
 * @param el_size Number of bytes that will be returned by each
 * pool_alloc (whose size parameter is then ignored). Can be 0 to
 * allow variable-sized allocations, but pool_free is then unusable.
 * @return LibError
 **/
extern LibError pool_create(Pool* p, size_t max_size, size_t el_size);

/**
 * free all memory (address space + physical) that constitutes the
 * given Pool.
 *
 * future alloc and free calls on this pool will fail.
 * continued use of the allocated memory (*) is
 * impossible because it is marked not-present via MMU.
 * (* no matter if in freelist or unused or "allocated" to user)
 *
 * @param Pool*
 * @return LibError.
 **/
extern LibError pool_destroy(Pool* p);

/**
 * indicate whether a pointer was allocated from the given pool.
 *
 * this is useful for callers that use several types of allocators.
 *
 * @param Pool*
 * @return bool.
 **/
extern bool pool_contains(Pool* p, void* el);

/**
 * Dole out memory from the pool.
 * exhausts the freelist before returning new entries to improve locality.
 *
 * @param Pool*
 * @param size bytes to allocate; ignored if pool_create's el_size was not 0.
 * @return allocated memory, or 0 if the Pool would have to be expanded and
 * there isn't enough memory to do so.
 **/
extern void* pool_alloc(Pool* p, size_t size);

/**
 * Make a fixed-size element available for reuse in the given Pool.
 *
 * this is not allowed if the Pool was created for variable-size elements.
 * rationale: avoids having to pass el_size here and compare with size when
 * allocating; also prevents fragmentation and leaking memory.
 *
 * @param Pool*
 * @param el Element returned by pool_alloc.
 **/
extern void pool_free(Pool* p, void* el);

/**
 * "free" all user allocations that ensued from the given Pool.
 *
 * this resets it as if freshly pool_create-d, but doesn't release the
 * underlying reserved virtual memory.
 *
 * @param Pool*
 **/
extern void pool_free_all(Pool* p);


//
// bucket allocator
//

/**
 * allocator design goals:
 * - either fixed- or variable-sized blocks;
 * - allow freeing individual blocks if they are all fixed-size;
 * - never relocates;
 * - no fixed limit.
 *
 * note: this type of allocator is called "region-based" in the literature.
 * see "Reconsidering Custom Memory Allocation" (Berger, Zorn, McKinley).
 * if individual variable-size elements must be freeable, consider "reaps":
 * basically a combination of region and heap, where frees go to the heap and
 * allocs exhaust that memory first and otherwise use the region.
 *
 * opaque! do not read/write any fields!
 **/
struct Bucket
{
	/**
	 * currently open bucket.
	 **/
	u8* bucket;

	/**
	 * offset of free space at end of current bucket (i.e. # bytes in use).
	 **/
	size_t pos;

	void* freelist;

	size_t el_size : 16;

	/**
	 * records # buckets allocated; verifies the list of buckets is correct.
	 **/
	uint num_buckets : 16;
};


/**
 * ready the Bucket object for use.
 *
 * @param Bucket*
 * @param el_size 0 to allow variable-sized allocations (which cannot be
 * freed individually); otherwise, it specifies the number of bytes that
 * will be returned by bucket_alloc (whose size parameter is then ignored).
 * @return LibError.
 **/
extern LibError bucket_create(Bucket* b, size_t el_size);

/**
 * free all memory that ensued from <b>.
 *
 * future alloc and free calls on this Bucket will fail.
 *
 * @param Bucket*
 **/
extern void bucket_destroy(Bucket* b);

/**
 * Dole out memory from the Bucket.
 * exhausts the freelist before returning new entries to improve locality.
 *
 * @param Bucket*
 * @param size bytes to allocate; ignored if bucket_create's el_size was not 0.
 * @return allocated memory, or 0 if the Bucket would have to be expanded and
 * there isn't enough memory to do so.
 **/
extern void* bucket_alloc(Bucket* b, size_t size);

/**
 * make an entry available for reuse in the given Bucket.
 *
 * this is not allowed if created for variable-size elements.
 * rationale: avoids having to pass el_size here and compare with size when
 * allocating; also prevents fragmentation and leaking memory.
 *
 * @param Bucket*
 * @param el entry allocated via bucket_alloc.
 **/
extern void bucket_free(Bucket* b, void* el);


//
// matrix allocator
//

/**
 * allocate a 2D matrix accessible as matrix[col][row].
 *
 * takes care of the dirty work of allocating 2D matrices:
 * - aligns data
 * - only allocates one memory block, which is more efficient than
 *   malloc/new for each row.
 *
 * @param cols, rows: dimension (cols x rows)
 * @param el_size size [bytes] of a matrix cell
 * @return 0 if out of memory, otherwise matrix that should be cast to
 * type** (sizeof(type) == el_size). must be freed via matrix_free.
 **/
extern void** matrix_alloc(uint cols, uint rows, size_t el_size);

/**
 * free the given matrix.
 *
 * @param matrix allocated by matrix_alloc; no-op if 0.
 * callers will likely want to pass variables of a different type
 * (e.g. int**); they must be cast to void**.
 **/
extern void matrix_free(void** matrix);


//
// allocator optimized for single instances
//

/**
 * Allocate <size> bytes of zeroed memory.
 *
 * intended for applications that frequently alloc/free a single
 * fixed-size object. caller provides static storage and an in-use flag;
 * we use that memory if available and otherwise fall back to the heap.
 * if the application only has one object in use at a time, malloc is
 * avoided; this is faster and avoids heap fragmentation.
 *
 * note: thread-safe despite use of shared static data.
 *
 * @param storage Caller-allocated memory of at least <size> bytes
 * (typically a static array of bytes)
 * @param in_use_flag Pointer to a flag we set when <storage> is in-use.
 * @param size [bytes] to allocate
 * @return allocated memory (typically = <storage>, but falls back to
 * malloc if that's in-use), or 0 (with warning) if out of memory.
 **/
extern void* single_calloc(void* storage, volatile uintptr_t* in_use_flag, size_t size);

/**
 * Free a memory block that had been allocated by single_calloc.
 *
 * @param storage Exact value passed to single_calloc.
 * @param in_use_flag Exact value passed to single_calloc.
 * @param Exact value returned by single_calloc.
 **/
extern void single_free(void* storage, volatile uintptr_t* in_use_flag, void* p);

#ifdef __cplusplus

/**
 * C++ wrapper on top of single_calloc that's slightly easier to use.
 *
 * T must be POD (Plain Old Data) because it is memset to 0!
 **/
template<class T> class SingleAllocator
{
	T storage;
	volatile uintptr_t is_in_use;

public:
	SingleAllocator()
	{
		is_in_use = 0;
	}

	T* alloc()
	{
		return (T*)single_calloc(&storage, &is_in_use, sizeof(storage));
	}

	void release(T* p)
	{
		single_free(&storage, &is_in_use, p);
	}
};

#endif	// #ifdef __cplusplus


//
// static allocator
//

// dole out chunks of memory from storage reserved in the BSS.
// freeing isn't necessary.

/**
 * opaque; initialized by STATIC_STORAGE and used by static_calloc
 **/
struct StaticStorage
{
	void* pos;
	void* end;
};

// define <size> bytes of storage and prepare <name> for use with
// static_calloc.
// must be invoked from file or function scope.
#define STATIC_STORAGE(name, size)\
	static u8 storage[(size)];\
	static StaticStorage name = { storage, storage+(size) }

/*
usage example:
static Object* pObject;
void InitObject()
{
	STATIC_STORAGE(ss, 100);	// includes padding
	void* addr = static_calloc(ss, sizeof(Object));
	pObject = new(addr) Object;
}
*/

/**
 * dole out memory from static storage reserved in BSS.
 *
 * this is useful for static objects that are used before _cinit - callers
 * define static storage for one or several objects, use this function to
 * retrieve an aligned pointer, then construct there via placement new.
 *
 * @param ss - initialized via STATIC_STORAGE
 * @param size [bytes] to allocate
 * @return aligned (suitable for any type) pointer
 *
 * raises a warning if there's not enough room (indicates incorrect usage)
 **/
extern void* static_calloc(StaticStorage* ss, size_t size);

// (no need to free static_calloc-ed memory since it's in the BSS)


//
// overrun protection
//

/**
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
**/
#ifdef REDEFINED_NEW
# include "lib/nommgr.h"
#endif
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
			WARN_ERR(ERR::NO_MEM);
			return;
		}
		if(da_set_size(&da, sizeof(T)) < 0)
			goto fail;

		cached_ptr = new(da.base) T();
		lock();
	}

	void shutdown()
	{
		if(!cpu_CAS(&initialized, 1, 2))
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
		if(cpu_CAS(&initialized, 0, 1))
			init();
		debug_assert(initialized != 2 && "OverrunProtector: used after dtor called:");
		unlock();
		return cached_ptr;
	}
};
#ifdef REDEFINED_NEW
# include "lib/mmgr.h"
#endif


//
// allocator test rig
//

/**
 * allocator test rig.
 * call from each allocator operation to sanity-check them.
 * should only be used during debug mode due to serious overhead.
 **/
class AllocatorChecker
{
public:
	void notify_alloc(void* p, size_t size)
	{
		const Allocs::value_type item = std::make_pair(p, size);
		std::pair<Allocs::iterator, bool> ret = allocs.insert(item);
		debug_assert(ret.second == true);	// wasn't already in map
	}

	void notify_free(void* p, size_t size)
	{
		Allocs::iterator it = allocs.find(p);
		if(it == allocs.end())
			debug_warn("AllocatorChecker: freeing invalid pointer");
		else
		{
			// size must match what was passed to notify_alloc
			const size_t allocated_size = it->second;
			debug_assert(size == allocated_size);

			allocs.erase(it);
		}
	}

	/**
	 * allocator is resetting itself, i.e. wiping out all allocs.
	 **/
	void notify_clear()
	{
		allocs.clear();
	}

private:
	typedef std::map<void*, size_t> Allocs;
	Allocs allocs;
};

#endif	// #ifndef INCLUDED_ALLOCATORS
