// malloc layer for less fragmentation, alignment, and automatic release

#include "precompiled.h"

#include "lib.h"
#include "res.h"

#include <stdlib.h>
#include <assert.h>

#include <map>


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
};

H_TYPE_DEFINE(Mem);


//////////////////////////////////////////////////////////////////////////////


static bool has_shutdown = false;

// raw pointer -> Handle
typedef std::map<void*, Handle> PtrToH;
typedef PtrToH::iterator It;
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
static Handle find_alloc(void* target_p, It* out_it = 0)
{
	// early out optimization (don't pay for full subset check)
	It it = ptr_to_h.find(target_p);
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
				{
					if(out_it)
						*out_it = it;
					return hm;
				}
			}
		}
	}

	// not found
	return 0;
}


// raw_p must be in map!
static void remove_alloc(void* raw_p)
{
	size_t num_removed = ptr_to_h.erase(raw_p);
	assert(num_removed == 1 && "remove_alloc: not in map");
}


// raw_p must not already be in map!
static void set_alloc(void* raw_p, Handle hm)
{
	// verify it's not already in the mapping
#ifndef NDEBUG
	It it = ptr_to_h.find(raw_p);
	if(it != ptr_to_h.end())
	{
		debug_warn("set_alloc: already in map");
		return;
	}
#endif

	ptr_to_h[raw_p] = hm;
}


//////////////////////////////////////////////////////////////////////////////



static void Mem_init(Mem* m, va_list args)
{
	// HACK: we pass along raw_p from h_alloc for use in Mem_reload
	// (that means add/remove from mapping code is only here)
	m->raw_p = va_arg(args, void*);
}


static void Mem_dtor(Mem* m)
{
	remove_alloc(m->raw_p);

	if(m->dtor)
		m->dtor(m->raw_p, m->raw_size, m->ctx);
}


// can't alloc here, because h_alloc needs the key when called
// (key == pointer we allocate)
static int Mem_reload(Mem* m, const char* fn, Handle hm)
{
	UNUSED(fn);

	set_alloc(m->raw_p, hm);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

// "*": aligned memory returned by allocator.
// "user_*": same as above, until someones changes it via mem_assign_user

// allocator interface:
// alloc: return at least size bytes of memory (alignment done by caller)

//////////////////////////////////////////////////////////////////////////////


static void heap_free(void* raw_p, size_t raw_size, uintptr_t ctx)
{
	UNUSED(raw_size);
	UNUSED(ctx);

	free(raw_p);
}


static void* heap_alloc(size_t raw_size, uintptr_t& ctx)
{
	ctx = 0;
	void* raw_p = malloc(raw_size);
	return raw_p;
}


//////////////////////////////////////////////////////////////////////////////


static u8* pool;
static size_t pool_pos;
static const size_t POOL_CAP = 8*MB;	// TODO: user editable


static void pool_free(void* raw_p, size_t raw_size, uintptr_t ctx)
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
		;	// TODO: warn about lost memory in pool;
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
	void* raw_p = (u8*)pool + ctx;
	return raw_p;
}


//////////////////////////////////////////////////////////////////////////////


int mem_free_h(Handle& hm)
{
	return h_free(hm, H_Mem);
}


int mem_free_p(void*& p)
{
	if(!p)
		return 0;

	Handle hm = find_alloc(p);
	p = 0;
	if(hm <= 0)
	{
		debug_warn("mem_free_p: not found in map");
		return -1;
	}
	return mem_free_h(hm);
}


// create a H_MEM handle of type MEM_USER,
// and assign it the specified memory range.
// dtor is called when the handle is freed, if non-NULL.
Handle mem_assign(void* p, size_t size, uint flags, void* raw_p, size_t raw_size, MEM_DTOR dtor, uintptr_t ctx)
{
	// we've already allocated that pointer - returns its handle
	Handle hm = find_alloc(p);
	if(hm > 0)
		return hm;

	if(!p || !size)
	{
		debug_warn("mem_assign: invalid p or size");
		return 0;
	}

	hm = h_alloc(H_Mem, (const char*)p, flags|RES_KEY|RES_NO_CACHE, raw_p);
	if(!hm)
		return 0;

	H_DEREF(hm, Mem, m);
	m->p         = p;
	m->size      = size;
	m->raw_p     = raw_p;
	m->raw_size  = raw_size;
	m->dtor      = dtor;
	m->ctx       = ctx;

	return hm;
}


/*
int mem_assign_user(Handle hm, void* user_p, size_t user_size)
{
	H_DEREF(hm, Mem, m);

	// security check: must be a subset of the existing buffer
	// (otherwise, could reference other buffers / cause mischief)
	char* raw_end  = (char*)m->raw_p + m->raw_size;
	char* user_end = (char*)user_p + user_size;
	if(user_p < m->raw_p || user_end > raw_end)
	{
		debug_warn("mem_assign_user: user buffer not contained in real buffer");
		return -EINVAL;
	}

	m->p = user_p;
	m->size = user_size;
	return 0;
}
*/


void* mem_alloc(size_t size, const size_t align, uint flags, Handle* phm)
{
	if(phm)
		*phm = ERR_NO_MEM;

	// note: this is legitimate. vfs_load on 0-length files must return
	// a valid and unique pointer to an (at least) 0-length buffer.
	if(size == 0)
		size = 1;

	// no scope indicated
	if(!flags)
		// in a handle _reload function - default to its scope
		if(res_cur_scope)
			flags = res_cur_scope;
		// otherwise, assume global scope


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


	Handle hm = mem_assign(p, size, flags, raw_p, raw_size, dtor, ctx);
	if(!hm)			// failed to allocate a handle
	{
		debug_warn("mem_alloc: mem_assign failed");
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
	Mem* m = H_USER_DATA(hm, Mem);
	if(!m)
	{
		if(user_size)
			*user_size = 0;
		return 0;
	}

	assert((!m->p || m->size) && "mem_get_ptr: mem corrupted (p valid =/=> size > 0)");

	if(user_size)
		*user_size = m->size;
	return m->p;
}


/*
ssize_t mem_size(void* p)
{
	Handle hm = find_alloc(p);
	H_DEREF(hm, Mem, m);
	return (ssize_t)m->size;
}
*/
