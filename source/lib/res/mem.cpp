// malloc layer for less fragmentation, alignment, and automatic release

#include "precompiled.h"

#include "lib.h"
#include "res.h"
#include "h_mgr.h"

#include <stdlib.h>
#include <assert.h>

#include <map>



struct Mem
{
	// what is reported by mem_get_ptr / mem_size
	void* user_p;
	size_t user_size;

	// actual allocation params (set by allocator)
	void* raw_p;
	size_t raw_size;
	uintptr_t ctx;

	MEM_DTOR dtor;	// this allows user-specified dtors.
};

H_TYPE_DEFINE(Mem);


static void Mem_init(Mem* m, va_list args)
{
	UNUSED(m);
	UNUSED(args);
}


static void Mem_dtor(Mem* m)
{
	if(m->dtor)
		m->dtor(m->raw_p, m->raw_size, m->ctx);
}


// can't alloc here, because h_alloc needs the key when called
// (key == pointer we allocate)
static int Mem_reload(Mem* /*m*/, const char* /*fn*/, Handle /*h*/)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

// "raw_*":  memory requested from allocator (+ padding for alignment
//           requested by mem_alloc)
// "user_*": same as raw, until someones changes it via {?}

// allocator interface:
// alloc: return at least raw_size bytes of memory (alignment done by caller)

//////////////////////////////////////////////////////////////////////////////


static void heap_free(void* const raw_p, const size_t raw_size, const uintptr_t ctx)
{
	UNUSED(raw_p);
	UNUSED(raw_size);

	void* heap_p = (void*)ctx;
	free(heap_p);
}


static void* heap_alloc(const size_t raw_size, uintptr_t& ctx)
{
	void* heap_p = malloc(raw_size);

	ctx = (uintptr_t)heap_p;
	return heap_p;
}


//////////////////////////////////////////////////////////////////////////////


static u8* pool;
static size_t pool_pos;
static const size_t POOL_CAP = 8*MB;	// TODO: user editable


static void pool_free(void* const raw_p, const size_t raw_size, const uintptr_t ctx)
{
	UNUSED(raw_p);

	size_t ofs = (size_t)ctx;

	// at or beyond current next-alloc position: invalid
	if(ofs >= pool_pos)
		debug_warn("pool_free: invalid ctx, beyond end of pool");
	// at end of pool; 'free' it by moving pos back
	else if(ofs + raw_size == pool_pos)
		pool_pos = ofs;
	else
		;	// TODO: warn about 'leaked' memory;
			// suggest using a different allocator
}


static void* pool_alloc(const size_t raw_size, uintptr_t& ctx)
{
	ctx = ~0;	// make sure it's invalid if we fail

	if(!pool)
	{
		pool = (u8*)mem_alloc(POOL_CAP, 64*KB, RES_STATIC);
		if(!pool)
			return 0;
	}

	if(pool_pos + raw_size > POOL_CAP)
	{
		debug_warn("pool_alloc: not enough memory in pool");
		return 0;
	}

	ctx = (uintptr_t)pool_pos;
	pool_pos += raw_size;
	return (u8*)pool + ctx;
}


//////////////////////////////////////////////////////////////////////////////


static bool has_shutdown = false;

typedef std::map<void*, Handle> PtrToH;
static PtrToH* _ptr_to_h;


static void ptr_to_h_shutdown()
{
	has_shutdown = true;
	delete _ptr_to_h;
	_ptr_to_h = 0;
}


// undefined NLSO init order fix
static PtrToH& get_ptr_to_h()
{
	if(!_ptr_to_h)
	{
		if(has_shutdown)
			debug_warn("mem.cpp: ptr -> handle lookup used after module shutdown");
			// crash + burn

		_ptr_to_h = new PtrToH;

		atexit2(ptr_to_h_shutdown);
	}
	return *_ptr_to_h;
}
#define ptr_to_h get_ptr_to_h()


// not needed by other modules - mem_get_size and mem_assign is enough.
static Handle find_alloc(void* target_p)
{
	// early out optimization (don't pay for full subset check)
	PtrToH::const_iterator it = ptr_to_h.find(target_p);
	if(it != ptr_to_h.end())
		return it->second;

	// not found; now check if target_p is within one of the mem ranges
	for(it = ptr_to_h.begin(); it != ptr_to_h.end(); ++it)
	{
		std::pair<void*, Handle> item = *it;
		void* p = item.first;
		Handle hm = item.second;

		// not before this alloc's p; could be it. now do range check.
		if(target_p >= p)
		{
			Mem* m = (Mem*)h_user_data(hm, H_Mem);
			if(m)
			{
				// found it within this mem range.
				if(target_p <= (char*)m->raw_p + m->raw_size)
					return hm;
			}
		}
	}

	// not found
	return 0;
}


// returns the handle that will be removed, for mem_free convenience.
// makes sure p is in the mapping.
static Handle remove_alloc(void* p)
{
	PtrToH::iterator it = ptr_to_h.find(p);
	if(it == ptr_to_h.end())
	{
		debug_warn("remove_alloc: pointer not in map");
		return 0;
	}

	Handle hm = it->second;
	ptr_to_h.erase(it);
	return hm;
}


// p must not alread be in mapping!
static void set_alloc(void* p, Handle hm)
{
	ptr_to_h[p] = hm;
}


//////////////////////////////////////////////////////////////////////////////



int mem_free_p(void*& p)
{
	if(!p)
		return ERR_INVALID_PARAM;

	Handle hm = remove_alloc(p);
	p = 0;

	return h_free(hm, H_Mem);
}


int mem_free_h(Handle& hm)
{
	void* p = mem_get_ptr(hm);
	hm = 0;
	return mem_free_p(p);
}


Handle mem_assign(void* raw_p, size_t raw_size, uint flags /* = 0 */, MEM_DTOR dtor /* = 0 */, uintptr_t ctx /* = 0 */)
{
	// we've already allocated that pointer - returns its handle
	Handle hm = find_alloc(raw_p);
	if(hm > 0)
		return hm;

	if(!raw_p || !raw_size)
	{
		debug_warn("mem_assign: invalid p or size");
		return 0;
	}

	hm = h_alloc(H_Mem, (const char*)raw_p, flags | RES_KEY);
	if(!hm)
		return 0;

	set_alloc(raw_p, hm);

	H_DEREF(hm, Mem, m);
	m->raw_p     = raw_p;
	m->raw_size  = raw_size;
	m->user_p    = raw_p;
	m->user_size = raw_size;
	m->dtor      = dtor;
	m->ctx       = ctx;

	return hm;
}


int mem_assign_user(Handle hm, void* user_p, size_t user_size)
{
	H_DEREF(hm, Mem, m);

	// make sure it's not already been assigned
	// (doesn't make sense, probably logic error)
	if(m->user_p != m->raw_p || m->user_size != m->raw_size)
	{
		debug_warn("mem_assign_user: already user_assign-ed");
		return -1;
	}

	// security check: must be a subset of the existing buffer
	// (otherwise, could reference other buffers / cause mischief)
	char* raw_end  = (char*)m->raw_p  + m->raw_size;
	char* user_end = (char*)m->user_p + m->user_size;
	if(user_p < m->raw_p || user_end > raw_end)
	{
		debug_warn("mem_assign_user: user buffer not contained in real buffer");
		return -EINVAL;
	}

	m->user_p = user_p;
	m->user_size = user_size;
	return 0;
}


void* mem_alloc(size_t size, const size_t align, uint flags, Handle* phm)
{
	if(phm)
		*phm = 0;

	if(size == 0)
	{
		debug_warn("mem_alloc: why is size = 0?");
		size = 1;
	}

	// no scope indicated
	if(!flags)
		// in a handle _reload function - default to its scope
		if(res_cur_scope)
			flags = res_cur_scope;
		// otherwise, assume global scope

	size_t raw_size = (size_t)round_up(size, align);

	void* raw_p;
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

	Handle hm = mem_assign(raw_p, raw_size, flags, dtor, ctx);
	if(!hm)			// failed to allocate a handle
	{
		dtor(raw_p, raw_size, ctx);
		return 0;
	}

	// check if pointer was already allocated?


	// caller is asking for the handle
	// (freeing the memory via handle is faster than mem_free, because
	//  we wouldn't have to scan all handles looking for the pointer)
	if(phm)
		*phm = hm;

	if(flags & MEM_ZERO)
		memset(raw_p, 0, raw_size);

	return raw_p;
}


void* mem_get_ptr(Handle hm, size_t* user_size /* = 0 */)
{
	Mem* m = H_USER_DATA(hm, Mem);
	if(!m)
	{
		if(user_size)
			*user_size = 0;
		return 0;
	}

	assert((!m->user_p || m->user_size) && "mem_get_ptr: mem corrupted (p valid =/=> size > 0)");

	if(user_size)
		*user_size = m->user_size;
	return m->user_p;
}


ssize_t mem_size(void* p)
{
	Handle hm = find_alloc(p);
	H_DEREF(hm, Mem, m);
	return (ssize_t)m->user_size;
}
