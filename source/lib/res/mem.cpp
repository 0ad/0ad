// malloc layer for less fragmentation, alignment, and automatic release

#include "precompiled.h"

#include "lib.h"
#include "res.h"
#include "h_mgr.h"

#include <stdlib.h>
#include <assert.h>

#include <map>

//////////////////////////////////////////////////////////////////////////////


static void heap_free(void* const p, const size_t size, const uintptr_t ctx)
{
	void* org_p = (void*)ctx;
	free(org_p);
}


static void* heap_alloc(const size_t size, const size_t align, uintptr_t& ctx, MEM_DTOR& dtor)
{
	u8* org_p = (u8*)malloc(size+align-1);
	u8* p = (u8*)round_up((uintptr_t)org_p, align);

	ctx = (uintptr_t)org_p;	
	dtor = heap_free;
	return p;
}


//////////////////////////////////////////////////////////////////////////////


static u8* pool;
static size_t pool_pos;
static const size_t POOL_CAP = 8*MB;	// TODO: user editable


static void pool_free(void* const p, const size_t size, const uintptr_t ctx)
{
	size_t ofs = ctx;

	// at end of pool? if so, 'free' it
	if(ofs + size == pool_pos)
		pool_pos -= size;
	else
		;	// TODO: warn about 'leaked' memory;
			// suggest using a different allocator
}


static void* pool_alloc(const size_t size, const size_t align, uintptr_t& ctx, MEM_DTOR& dtor)
{
	if(!pool)
	{
		pool = (u8*)mem_alloc(size, align, 0);
		if(!pool)
			return 0;
	}

	ptrdiff_t ofs = round_up(pool_pos, align);
	ctx = (uintptr_t)ofs;
	dtor = pool_free;

	if(ofs+size > POOL_CAP)
	{
		debug_warn("pool_alloc: not enough memory in pool");
		return 0;
	}

	pool_pos = ofs+size;
	return (u8*)pool + ofs;
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
static Handle find_alloc(void* p)
{
	PtrToH::const_iterator it = ptr_to_h.find(p);
	if(it == ptr_to_h.end())
		return 0;
	return it->second;
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


struct Mem
{
	void* p;
	size_t size;

	// allocator specific

	uintptr_t ctx;

	MEM_DTOR dtor;	// this allows user-specified dtors.
					// alternative: switch(mem->type) in mem_dtor
};

H_TYPE_DEFINE(Mem)


static void Mem_init(Mem* m, va_list args)
{
}


static void Mem_dtor(Mem* m)
{
	if(m->dtor)
		m->dtor(m->p, m->size, m->ctx);
}


// can't alloc here, because h_alloc needs the key when called
// (key == pointer we allocate)
static int Mem_reload(Mem* m, const char* fn)
{
	return 0;
}


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


Handle mem_assign(void* p, size_t size, uint flags /* = 0 */, MEM_DTOR dtor /* = 0 */, uintptr_t ctx /* = 0 */)
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

	hm = h_alloc(H_Mem, (const char*)p, flags | RES_KEY);
	if(!hm)
		return 0;

	set_alloc(p, hm);

	H_DEREF(hm, Mem, m);
	m->p    = p;
	m->size = size;
	m->dtor = dtor;
	m->ctx  = ctx;

	return hm;
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

	int scope = flags & RES_SCOPE_MASK;

	// filled in by allocators
	uintptr_t ctx;
	MEM_DTOR dtor;

	void* p = 0;
	if(scope == RES_TEMP)
		p = pool_alloc(size, align, ctx, dtor);
	else
		p = heap_alloc(size, align, ctx, dtor);
	if(!p)
		return 0;

	Handle hm = mem_assign(p, size, scope, dtor, ctx);
	if(!hm)			// failed to allocate a handle
	{
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


void* mem_get_ptr(Handle hm, size_t* size /* = 0 */)
{
	Mem* m = H_USER_DATA(hm, Mem);
	if(!m)
	{
		if(size)
			*size = 0;
		return 0;
	}

	assert((!m->p || m->size) && "mem_get_ptr: mem corrupted (p valid =/=> size > 0)");

	if(size)
		*size = m->size;
	return m->p;
}


ssize_t mem_size(void* p)
{
	Handle hm = find_alloc(p);
	H_DEREF(hm, Mem, m);
	return (ssize_t)m->size;
}
