/**
 * =========================================================================
 * File        : mem.cpp
 * Project     : 0 A.D.
 * Description : wrapper that treats memory as a "resource", i.e.
 *             : guarantees its lifetime and automatic release.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "mem.h"

#include <stdlib.h>
#include <map>

#include "lib/posix/posix_pthread.h"
#include "lib/bits.h"
#include "lib/allocators.h"	// OverrunProtector
#include "h_mgr.h"


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct ScopedLock
{
	ScopedLock() { pthread_mutex_lock(&mutex); }
	~ScopedLock() { pthread_mutex_unlock(&mutex); }	
};
#define SCOPED_LOCK ScopedLock UID__;

struct Mem
{
	// initially what mem_alloc returns; can be changed via mem_assign_user
	void* p;
	size_t size;

	// unaligned mem from allocator
	void* raw_p;
	size_t raw_size;

	uintptr_t ctx;
	MEM_DTOR dtor;	// this allows user-specified dtors.

	void* owner;
};

H_TYPE_DEFINE(Mem);


//////////////////////////////////////////////////////////////////////////////


// raw pointer -> Handle
typedef std::map<void*, Handle> Ptr2H;
typedef Ptr2H::iterator It;
static OverrunProtector<Ptr2H> ptr2h_wrapper;

// not needed by other modules - mem_get_size and mem_wrap is enough.
static Handle find_alloc(void* target_raw_p, It* out_it = 0)
{
	Ptr2H* ptr2h = ptr2h_wrapper.get();
	if(!ptr2h)
		WARN_RETURN(ERR::NO_MEM);

	// early out optimization (don't pay for full subset check)
	It it = ptr2h->find(target_raw_p);
	if(it != ptr2h->end())
		return it->second;

	// initial return value: "not found"
	Handle ret = -1;

	// not found; now check if target_raw_p is within one of the mem ranges
	for(it = ptr2h->begin(); it != ptr2h->end(); ++it)
	{
		void* raw_p = it->first;
		Handle hm   = it->second;

		// not before this alloc's p; could be it. now do range check.
		if(target_raw_p >= raw_p)
		{
			Mem* m = (Mem*)h_user_data(hm, H_Mem);
			if(m)
			{
				// found it within this mem range.
				if(target_raw_p <= (char*)m->raw_p + m->raw_size)
				{
					if(out_it)
						*out_it = it;
					ret = hm;
					break;
				}
			}
		}
	}

	ptr2h_wrapper.lock();
	return ret;
}


// raw_p must be in map!
static void remove_alloc(void* raw_p)
{
	Ptr2H* ptr2h = ptr2h_wrapper.get();
	if(!ptr2h)
		return;

//debug_printf("REMOVE   raw_p=%p\n", raw_p);

	size_t num_removed = ptr2h->erase(raw_p);
	if(num_removed != 1)
		debug_warn("not in map");

	ptr2h_wrapper.lock();
}


// raw_p must not already be in map!
static void set_alloc(void* raw_p, Handle hm)
{
	Ptr2H* ptr2h = ptr2h_wrapper.get();
	if(!ptr2h)
		return;

//debug_printf("SETALLOC raw_p=%p hm=%I64x\n", raw_p, hm);

	std::pair<It, bool> ret = ptr2h->insert(std::make_pair(raw_p, hm));
	if(!ret.second)
		debug_warn("already in map");

	ptr2h_wrapper.lock();
}


//////////////////////////////////////////////////////////////////////////////



static void Mem_init(Mem* m, va_list args)
{
	// these are passed to h_alloc instead of assigning in mem_wrap after a
	// H_DEREF so that Mem_validate won't complain about invalid (0) values.
	//
	// additional bonus: by setting raw_p before reload, that and the
	// dtor will be the only call site of set/remove_alloc.
	m->p         = va_arg(args, void*);
	m->size      = va_arg(args, size_t);
	m->raw_p     = va_arg(args, void*);
	m->raw_size  = va_arg(args, size_t);
	m->dtor      = va_arg(args, MEM_DTOR);
	m->ctx       = va_arg(args, uintptr_t);
	m->owner     = va_arg(args, void*);
}

static void Mem_dtor(Mem* m)
{
	// (reload can't fail)

	remove_alloc(m->raw_p);

	if(m->dtor)
		m->dtor(m->raw_p, m->raw_size, m->ctx);
}

// can't alloc here, because h_alloc needs the key when called
// (key == pointer we allocate)
static LibError Mem_reload(Mem* m, const char* UNUSED(fn), Handle hm)
{
	set_alloc(m->raw_p, hm);
	return INFO::OK;
}

static LibError Mem_validate(const Mem* m)
{
	if(debug_is_pointer_bogus(m->p))
		WARN_RETURN(ERR::_1);
	if(!m->size)
		WARN_RETURN(ERR::_2);
	if(m->raw_p && m->raw_p > m->p)
		WARN_RETURN(ERR::_3);
	if(m->raw_size && m->raw_size < m->size)
		WARN_RETURN(ERR::_4);
	return INFO::OK;
}

static LibError Mem_to_string(const Mem* m, char* buf)
{
	char owner_sym[DBG_SYMBOL_LEN];
	if(debug_resolve_symbol(m->owner, owner_sym, 0, 0) < 0)
	{
		if(m->owner)
			snprintf(owner_sym, ARRAY_SIZE(owner_sym), "(%p)", m->owner);
		else
			strcpy_s(owner_sym, ARRAY_SIZE(owner_sym), "(?)");
	}

	snprintf(buf, H_STRING_LEN, "p=%p size=%d owner=%s", m->p, m->size, owner_sym);
	return INFO::OK;
}


//////////////////////////////////////////////////////////////////////////////

// "*": aligned memory returned by allocator.
// "user_*": same as above, until someones changes it via mem_assign_user

// allocator interface:
// alloc: return at least size bytes of memory (alignment done by caller)

//////////////////////////////////////////////////////////////////////////////

// implementation must be thread-safe! (since mem_alloc doesn't take a lock)

static void heap_free(void* raw_p, size_t UNUSED(raw_size), uintptr_t UNUSED(ctx))
{
	free(raw_p);
}


static void* heap_alloc(size_t raw_size, uintptr_t& ctx)
{
	ctx = 0;
	void* raw_p = malloc(raw_size);
	return raw_p;
}


//////////////////////////////////////////////////////////////////////////////


LibError mem_free_h(Handle& hm)
{
	SCOPED_LOCK;
	return h_free(hm, H_Mem);
}


LibError mem_free_p(void*& p)
{
	if(!p)
		return INFO::OK;

	Handle hm;
	{
	SCOPED_LOCK;
	hm = find_alloc(p);
	}

	p = 0;
	if(hm <= 0)
		WARN_RETURN(ERR::MEM_ALLOC_NOT_FOUND);
	return mem_free_h(hm);
}


// create a H_MEM handle of type MEM_USER,
// and assign it the specified memory range.
// if dtor is non-NULL, it is called (passing ctx) when the handle is freed.
Handle mem_wrap(void* p, size_t size, uint flags, void* raw_p, size_t raw_size, MEM_DTOR dtor, uintptr_t ctx, void* owner)
{
	if(!p || !size)
		WARN_RETURN(ERR::INVALID_PARAM);

	SCOPED_LOCK;

	// we've already allocated that pointer; return its handle and
	// increment refcnt.
	Handle hm = find_alloc(p);
	if(hm > 0)
	{
		h_add_ref(hm);
		return hm;
	}

	// <p> wasn't allocated via mem_alloc, or we would've found its Handle.
	// it is therefore some user-allocated mem and might therefore not have
	// a valid <raw_p> set. since that's our search key, we set it to <p>.
	if(!raw_p)
		raw_p = p;
	if(!raw_size)
		raw_size = size;

	hm = h_alloc(H_Mem, (const char*)p, flags|RES_KEY|RES_NO_CACHE,
		p, size, raw_p, raw_size, dtor, ctx, owner);
	return hm;
}


/*
LibError mem_assign_user(Handle hm, void* user_p, size_t user_size)
{
	H_DEREF(hm, Mem, m);

	// security check: must be a subset of the existing buffer
	// (otherwise, could reference other buffers / cause mischief)
	char* raw_end  = (char*)m->raw_p + m->raw_size;
	char* user_end = (char*)user_p + user_size;
	if(user_p < m->raw_p || user_end > raw_end)
	{
		debug_warn("mem_assign_user: user buffer not contained in real buffer");
		return -1;
	}

	m->p = user_p;
	m->size = user_size;
	return INFO::OK;
}
*/


// implementation note: does not currently take a lock; all the
// heavy-lifting happens inside mem_wrap.
void* mem_alloc(size_t size, const size_t align, uint flags, Handle* phm)
{
	if(phm)
		*phm  = ERR::NO_MEM;

#ifdef NDEBUG
	void* owner = 0;
#else
	void* owner = debug_get_nth_caller(1, 0);
#endif

	// note: this is legitimate. vfs_load on 0-length files must return
	// a valid and unique pointer to an (at least) 0-length buffer.
	if(size == 0)
	{
		debug_printf("MEM 0 byte alloc\n");
		size = 1;
	}

	void* raw_p;
	const size_t raw_size = size + align-1;

	uintptr_t ctx;
	MEM_DTOR dtor;

//	if(scope == RES_TEMP)
//	{
//		raw_p = pool_alloc(raw_size, ctx);
//		dtor = pool_free;
//	}
//	else
	{
		raw_p = heap_alloc(raw_size, ctx);
		dtor = heap_free;
	}

	if(!raw_p)
		return 0;
	void* p = (void*)round_up((uintptr_t)raw_p, align);

//debug_printf("MEMWRAP  p=%p size=%x raw_p=%p raw_size=%x owner=%p\n", p, size, raw_p, raw_size, owner);
	Handle hm = mem_wrap(p, size, flags, raw_p, raw_size, dtor, ctx, owner);
	if(!hm)			// failed to allocate a handle
	{
		debug_warn("mem_wrap failed");
		dtor(p, size, ctx);
		return 0;
	}

	// check if pointer was already allocated?


	// caller is asking for the handle
	// (freeing the memory via handle is faster than mem_free, because
	//  we wouldn't have to scan all handles looking for the pointer)
	if(phm)
		*phm = hm;

	if(flags & MEM_ZERO)
		memset(p, 0, size);

	return p;
}


void* mem_get_ptr(Handle hm, size_t* user_size /* = 0 */)
{
	SCOPED_LOCK;

	Mem* m = H_USER_DATA(hm, Mem);
	if(!m)
	{
		if(user_size)
			*user_size = 0;
		return 0;
	}

	debug_assert((!m->p || m->size) && "mem_get_ptr: mem corrupted (p valid =/=> size > 0)");

	if(user_size)
		*user_size = m->size;
	return m->p;
}


LibError mem_get(Handle hm, u8** pp, size_t* psize)
{
	SCOPED_LOCK;

	H_DEREF(hm, Mem, m);
	if(pp)
		*pp = (u8*)m->p;
	if(psize)
		*psize = m->size;
	// leave hm locked
	return INFO::OK;
}


/*
ssize_t mem_size(void* p)
{
	SCOPED_LOCK;
	Handle hm = find_alloc(p);
	H_DEREF(hm, Mem, m);
	return (ssize_t)m->size;
}
*/


// exception to normal resource shutdown: must not be called before
// h_mgr_shutdown! (this is because h_mgr calls us to free memory, which
// requires the pointer -> Handle index still be in place)
void mem_shutdown()
{
	// ptr2h_wrapper is currently freed at NLSO dtor time.
	// if that's a problem, use OverrunProtector::shutdown.
}
