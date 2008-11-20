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

#include "lib/config2.h"	// CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
#include "lib/posix/posix_mman.h"	// PROT_*
#include "lib/sysdep/cpu.h"	// cpu_CAS


//
// page aligned allocator
//

/**
 * allocate memory aligned to the system page size.
 *
 * this is useful for file_cache_alloc, which uses this allocator to
 * get sector-aligned (hopefully; see sys_max_sector_size) IO buffers.
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
LIB_API void* page_aligned_alloc(size_t unaligned_size);

/**
 * free a previously allocated page-aligned region.
 *
 * @param p exact value returned from page_aligned_alloc
 * @param size exact value passed to page_aligned_alloc
 **/
LIB_API void page_aligned_free(void* p, size_t unaligned_size);

#ifdef __cplusplus

template<typename T>
class PageAlignedDeleter
{
public:
	PageAlignedDeleter(size_t size)
		: m_size(size)
	{
		debug_assert(m_size != 0);
	}

	void operator()(T* p)
	{
		debug_assert(m_size != 0);
		page_aligned_free(p, m_size);
		m_size = 0;
	}

private:
	size_t m_size;
};

template<typename T>
class PageAlignedAllocator
{
public:
	shared_ptr<T> operator()(size_t size) const
	{
		return shared_ptr<T>((T*)page_aligned_alloc(size), PageAlignedDeleter<T>(size));
	}
};

#endif


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
extern void** matrix_alloc(size_t cols, size_t rows, size_t el_size);

/**
 * free the given matrix.
 *
 * @param matrix allocated by matrix_alloc; no-op if 0.
 * callers will likely want to pass variables of a different type
 * (e.g. int**); they must be cast to void**.
 **/
extern void matrix_free(void** matrix);


//-----------------------------------------------------------------------------
// allocator optimized for single instances
//-----------------------------------------------------------------------------

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
	// evil but necessary hack: we don't want to instantiate a T directly
	// because it may not have a default ctor. an array of uninitialized
	// storage is used instead. single_calloc doesn't know about alignment,
	// so we fix this by asking for an array of doubles.
	double storage[(sizeof(T)+sizeof(double)-1)/sizeof(double)];
	volatile uintptr_t is_in_use;

public:
	typedef T value_type;

	SingleAllocator()
	{
		is_in_use = 0;
	}

	T* Allocate()
	{
		T* t = (T*)single_calloc(&storage, &is_in_use, sizeof(storage));
		if(!t)
			throw std::bad_alloc();
		return t;
	}

	void Free(T* p)
	{
		single_free(&storage, &is_in_use, p);
	}
};

#endif	// #ifdef __cplusplus


//-----------------------------------------------------------------------------
// static allocator
//-----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------
// OverrunProtector
//-----------------------------------------------------------------------------

/**
OverrunProtector wraps an arbitrary object in DynArray memory and can detect
inadvertent writes to it. this is useful for tracking down memory overruns.

the basic idea is to require users to request access to the object and
notify us when done; memory access permission is temporarily granted.
(similar in principle to Software Transaction Memory).

since this is quite slow, the protection is disabled unless
CONFIG2_ALLOCATORS_OVERRUN_PROTECTION == 1; this avoids having to remove the
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
template<class T> class OverrunProtector
{
public:
	OverrunProtector()
	{
		void* mem = page_aligned_alloc(sizeof(T));
		object = new(mem) T();
		lock();
	}

	~OverrunProtector()
	{
		unlock();
		object->~T();	// call dtor (since we used placement new)
		page_aligned_free(object, sizeof(T));
		object = 0;
	}

	T* get()
	{
		unlock();
		return object;
	}

	void lock()
	{
#if CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
		mprotect(object, sizeof(T), PROT_NONE);
#endif
	}

private:
	void unlock()
	{
#if CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
		mprotect(object, sizeof(T), PROT_READ|PROT_WRITE);
#endif
	}

	T* object;
};


//-----------------------------------------------------------------------------
// AllocatorChecker
//-----------------------------------------------------------------------------

/**
 * allocator test rig.
 * call from each allocator operation to sanity-check them.
 * should only be used during debug mode due to serious overhead.
 **/
class AllocatorChecker
{
public:
	void OnAllocate(void* p, size_t size)
	{
		const Allocs::value_type item = std::make_pair(p, size);
		std::pair<Allocs::iterator, bool> ret = allocs.insert(item);
		debug_assert(ret.second == true);	// wasn't already in map
	}

	void OnDeallocate(void* p, size_t size)
	{
		Allocs::iterator it = allocs.find(p);
		if(it == allocs.end())
			debug_assert(0);	// freeing invalid pointer
		else
		{
			// size must match what was passed to OnAllocate
			const size_t allocated_size = it->second;
			debug_assert(size == allocated_size);

			allocs.erase(it);
		}
	}

	/**
	 * allocator is resetting itself, i.e. wiping out all allocs.
	 **/
	void OnClear()
	{
		allocs.clear();
	}

private:
	typedef std::map<void*, size_t> Allocs;
	Allocs allocs;
};

#endif	// #ifndef INCLUDED_ALLOCATORS
