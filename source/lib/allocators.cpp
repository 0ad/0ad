/**
 * =========================================================================
 * File        : allocators.cpp
 * Project     : 0 A.D.
 * Description : memory suballocators.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "allocators.h"

#include "lib/posix/posix_mman.h"	// PROT_* constants for da_set_prot
#include "lib/posix/posix.h"		// sysconf
#include "lib/sysdep/cpu.h"	// cpu_CAS
#include "byte_order.h"
#include "bits.h"


//-----------------------------------------------------------------------------
// helper routines
//-----------------------------------------------------------------------------

// latch page size in case we are called from static ctors (it's possible
// that they are called before our static initializers).
// pool_create is therefore now safe to call before main().
static size_t get_page_size()
{
	static const size_t page_size = cpu_PageSize();
	return page_size;
}

static inline bool is_page_multiple(uintptr_t x)
{
	return (x % get_page_size()) == 0;
}

static inline size_t round_up_to_page(size_t size)
{
	return round_up(size, get_page_size());
}


// very thin wrapper on top of sys/mman.h that makes the intent more obvious:
// (its commit/decommit semantics are difficult to tell apart)

static inline LibError LibError_from_mmap(void* ret, bool warn_if_failed = true)
{
	if(ret != MAP_FAILED)
		return INFO::OK;
	return LibError_from_errno(warn_if_failed);
}

// "anonymous" effectively means mapping /dev/zero, but is more efficient.
// MAP_ANONYMOUS is not in SUSv3, but is a very common extension.
// unfortunately, MacOS X only defines MAP_ANON, which Solaris says is
// deprecated. workaround there: define MAP_ANONYMOUS in terms of MAP_ANON.
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif

static const int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS;

static LibError mem_reserve(size_t size, u8** pp)
{
	errno = 0;
	void* ret = mmap(0, size, PROT_NONE, mmap_flags|MAP_NORESERVE, -1, 0);
	*pp = (u8*)ret;
	return LibError_from_mmap(ret);
}

static LibError mem_release(u8* p, size_t size)
{
	errno = 0;
	int ret = munmap(p, size);
	return LibError_from_posix(ret);
}

static LibError mem_commit(u8* p, size_t size, int prot)
{
	if(prot == PROT_NONE)
		// not allowed - it would be misinterpreted by mmap.
		WARN_RETURN(ERR::INVALID_PARAM);

	errno = 0;
	void* ret = mmap(p, size, prot, mmap_flags|MAP_FIXED, -1, 0);
	return LibError_from_mmap(ret);
}

static LibError mem_decommit(u8* p, size_t size)
{
	errno = 0;
	void* ret = mmap(p, size, PROT_NONE, mmap_flags|MAP_NORESERVE|MAP_FIXED, -1, 0);
	return LibError_from_mmap(ret);
}

static LibError mem_protect(u8* p, size_t size, int prot)
{
	errno = 0;
	int ret = mprotect(p, size, prot);
	return LibError_from_posix(ret);
}


//-----------------------------------------------------------------------------
// page aligned allocator
//-----------------------------------------------------------------------------

void* page_aligned_alloc(size_t unaligned_size)
{
	const size_t size_pa = round_up_to_page(unaligned_size);
	u8* p = 0;
	RETURN0_IF_ERR(mem_reserve(size_pa, &p));
	RETURN0_IF_ERR(mem_commit(p, size_pa, PROT_READ|PROT_WRITE));
	return p;
}


void page_aligned_free(void* p, size_t unaligned_size)
{
	if(!p)
		return;
	debug_assert(is_page_multiple((uintptr_t)p));
	const size_t size_pa = round_up_to_page(unaligned_size);
	(void)mem_release((u8*)p, size_pa);
}


//-----------------------------------------------------------------------------
// dynamic (expandable) array
//-----------------------------------------------------------------------------

// indicates that this DynArray must not be resized or freed
// (e.g. because it merely wraps an existing memory range).
// stored in da->prot to reduce size; doesn't conflict with any PROT_* flags.
const int DA_NOT_OUR_MEM = 0x40000000;

static LibError validate_da(DynArray* da)
{
	if(!da)
		WARN_RETURN(ERR::INVALID_PARAM);
	u8* const base           = da->base;
	const size_t max_size_pa = da->max_size_pa;
	const size_t cur_size    = da->cur_size;
	const size_t pos         = da->pos;
	const int prot           = da->prot;

	if(debug_is_pointer_bogus(base))
		WARN_RETURN(ERR::_1);
	// note: don't check if base is page-aligned -
	// might not be true for 'wrapped' mem regions.
//	if(!is_page_multiple((uintptr_t)base))
//		WARN_RETURN(ERR::_2);
	if(!is_page_multiple(max_size_pa))
		WARN_RETURN(ERR::_3);
	if(cur_size > max_size_pa)
		WARN_RETURN(ERR::_4);
	if(pos > cur_size || pos > max_size_pa)
		WARN_RETURN(ERR::_5);
	if(prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC|DA_NOT_OUR_MEM))
		WARN_RETURN(ERR::_6);

	return INFO::OK;
}

#define CHECK_DA(da) RETURN_ERR(validate_da(da))


LibError da_alloc(DynArray* da, size_t max_size)
{
	const size_t max_size_pa = round_up_to_page(max_size);

	u8* p;
	RETURN_ERR(mem_reserve(max_size_pa, &p));

	da->base        = p;
	da->max_size_pa = max_size_pa;
	da->cur_size    = 0;
	da->cur_size_pa = 0;
	da->prot        = PROT_READ|PROT_WRITE;
	da->pos         = 0;
	CHECK_DA(da);
	return INFO::OK;
}


LibError da_free(DynArray* da)
{
	CHECK_DA(da);

	u8* p            = da->base;
	size_t size_pa   = da->max_size_pa;
	bool was_wrapped = (da->prot & DA_NOT_OUR_MEM) != 0;

	// wipe out the DynArray for safety
	// (must be done here because mem_release may fail)
	memset(da, 0, sizeof(*da));

	// skip mem_release if <da> was allocated via da_wrap_fixed
	// (i.e. it doesn't actually own any memory). don't complain;
	// da_free is supposed to be called even in the above case.
	if(!was_wrapped)
		RETURN_ERR(mem_release(p, size_pa));
	return INFO::OK;
}


LibError da_set_size(DynArray* da, size_t new_size)
{
	CHECK_DA(da);

	if(da->prot & DA_NOT_OUR_MEM)
		WARN_RETURN(ERR::LOGIC);

	// determine how much to add/remove
	const size_t cur_size_pa = round_up_to_page(da->cur_size);
	const size_t new_size_pa = round_up_to_page(new_size);
	const ssize_t size_delta_pa = (ssize_t)new_size_pa - (ssize_t)cur_size_pa;

	// not enough memory to satisfy this expand request: abort.
	// note: do not complain - some allocators (e.g. file_cache)
	// legitimately use up all available space.
	if(new_size_pa > da->max_size_pa)
		return ERR::LIMIT;	// NOWARN

	u8* end = da->base + cur_size_pa;
	// expanding
	if(size_delta_pa > 0)
		RETURN_ERR(mem_commit(end, size_delta_pa, da->prot));
	// shrinking
	else if(size_delta_pa < 0)
		RETURN_ERR(mem_decommit(end+size_delta_pa, -size_delta_pa));
	// else: no change in page count, e.g. if going from size=1 to 2
	// (we don't want mem_* to have to handle size=0)

	da->cur_size = new_size;
	da->cur_size_pa = new_size_pa;
	CHECK_DA(da);
	return INFO::OK;
}


LibError da_reserve(DynArray* da, size_t size)
{
	if(da->pos+size > da->cur_size_pa)
		RETURN_ERR(da_set_size(da, da->cur_size_pa+size));
	da->cur_size = std::max(da->cur_size, da->pos+size);
	return INFO::OK;
}


LibError da_set_prot(DynArray* da, int prot)
{
	CHECK_DA(da);

	// somewhat more subtle: POSIX mprotect requires the memory have been
	// mmap-ed, which it probably wasn't here.
	if(da->prot & DA_NOT_OUR_MEM)
		WARN_RETURN(ERR::LOGIC);

	da->prot = prot;
	RETURN_ERR(mem_protect(da->base, da->cur_size_pa, prot));

	CHECK_DA(da);
	return INFO::OK;
}


LibError da_wrap_fixed(DynArray* da, u8* p, size_t size)
{
	da->base        = p;
	da->max_size_pa = round_up_to_page(size);
	da->cur_size    = size;
	da->cur_size_pa = da->max_size_pa;
	da->prot        = PROT_READ|PROT_WRITE|DA_NOT_OUR_MEM;
	da->pos         = 0;
	CHECK_DA(da);
	return INFO::OK;
}


LibError da_read(DynArray* da, void* data, size_t size)
{
	// make sure we have enough data to read
	if(da->pos+size > da->cur_size)
		WARN_RETURN(ERR::FAIL);

	cpu_memcpy(data, da->base+da->pos, size);
	da->pos += size;
	return INFO::OK;
}


LibError da_append(DynArray* da, const void* data, size_t size)
{
	RETURN_ERR(da_reserve(da, size));
	cpu_memcpy(da->base+da->pos, data, size);
	da->pos += size;
	return INFO::OK;
}


//-----------------------------------------------------------------------------
// pool allocator
//-----------------------------------------------------------------------------

// "freelist" is a pointer to the first unused element (0 if there are none);
// its memory holds a pointer to the next free one in list.

static void freelist_push(void** pfreelist, void* el)
{
	debug_assert(el != 0);
	void* prev_el = *pfreelist;
	*pfreelist = el;
	*(void**)el = prev_el;
}

static void* freelist_pop(void** pfreelist)
{
	void* el = *pfreelist;
	// nothing in list
	if(!el)
		return 0;
	*pfreelist = *(void**)el;
	return el;
}


// elements returned are aligned to this many bytes:
static const size_t ALIGN = 8;


LibError pool_create(Pool* p, size_t max_size, size_t el_size)
{
	if(el_size == POOL_VARIABLE_ALLOCS)
		p->el_size = 0;
	else
		p->el_size = round_up(el_size, ALIGN);
	p->freelist = 0;
	RETURN_ERR(da_alloc(&p->da, max_size));
	return INFO::OK;
}


LibError pool_destroy(Pool* p)
{
	// don't be picky and complain if the freelist isn't empty;
	// we don't care since it's all part of the da anyway.
	// however, zero it to prevent further allocs from succeeding.
	p->freelist = 0;
	return da_free(&p->da);
}


bool pool_contains(Pool* p, void* el)
{
	// outside of our range
	if(!(p->da.base <= el && el < p->da.base+p->da.pos))
		return false;
	// sanity check: it should be aligned (if pool has fixed-size elements)
	if(p->el_size)
		debug_assert((uintptr_t)((u8*)el - p->da.base) % p->el_size == 0);
	return true;
}


void* pool_alloc(Pool* p, size_t size)
{
	// if pool allows variable sizes, go with the size parameter,
	// otherwise the pool el_size setting.
	const size_t el_size = p->el_size? p->el_size : round_up(size, ALIGN);

	// note: this can never happen in pools with variable-sized elements
	// because they disallow pool_free.
	void* el = freelist_pop(&p->freelist);
	if(el)
		goto have_el;

	// alloc a new entry
	{
		// expand, if necessary
		if(da_reserve(&p->da, el_size) < 0)
			return 0;

		el = p->da.base + p->da.pos;
		p->da.pos += el_size;
	}

have_el:
	debug_assert(pool_contains(p, el));	// paranoia
	return el;
}


void pool_free(Pool* p, void* el)
{
	// only allowed to free items if we were initialized with
	// fixed el_size. (this avoids having to pass el_size here and
	// check if requested_size matches that when allocating)
	if(p->el_size == 0)
	{
		debug_warn("cannot free variable-size items");
		return;
	}

	if(pool_contains(p, el))
		freelist_push(&p->freelist, el);
	else
		debug_warn("invalid pointer (not in pool)");
}


void pool_free_all(Pool* p)
{
	p->freelist = 0;

	// must be reset before da_set_size or CHECK_DA will complain.
	p->da.pos = 0;

	da_set_size(&p->da, 0);
}


//-----------------------------------------------------------------------------
// bucket allocator
//-----------------------------------------------------------------------------

// power-of-2 isn't required; value is arbitrary.
const size_t BUCKET_SIZE = 4000;


LibError bucket_create(Bucket* b, size_t el_size)
{
	b->freelist = 0;
	b->el_size = round_up(el_size, ALIGN);

	// note: allocating here avoids the is-this-the-first-time check
	// in bucket_alloc, which speeds things up.
	b->bucket = (u8*)malloc(BUCKET_SIZE);
	if(!b->bucket)
	{
		// cause next bucket_alloc to retry the allocation
		b->pos = BUCKET_SIZE;
		b->num_buckets = 0;
		WARN_RETURN(ERR::NO_MEM);
	}

	*(u8**)b->bucket = 0;	// terminate list
	b->pos = round_up(sizeof(u8*), ALIGN);
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
	b->el_size = BUCKET_SIZE;
}


void* bucket_alloc(Bucket* b, size_t size)
{
	size_t el_size = b->el_size? b->el_size : round_up(size, ALIGN);
	// must fit in a bucket
	debug_assert(el_size <= BUCKET_SIZE-sizeof(u8*));

	// try to satisfy alloc from freelist
	void* el = freelist_pop(&b->freelist);
	if(el)
		return el;

	// if there's not enough space left, close current bucket and
	// allocate another.
	if(b->pos+el_size > BUCKET_SIZE)
	{
		u8* bucket = (u8*)malloc(BUCKET_SIZE);
		if(!bucket)
			return 0;
		*(u8**)bucket = b->bucket;
		b->bucket = bucket;
		// skip bucket list field and align (note: malloc already
		// aligns to at least 8 bytes, so don't take b->bucket into account)
		b->pos = round_up(sizeof(u8*), ALIGN);
		b->num_buckets++;
	}

	void* ret = b->bucket+b->pos;
	b->pos += el_size;
	return ret;
}


void bucket_free(Bucket* b, void* el)
{
	if(b->el_size == 0)
	{
		debug_warn("cannot free variable-size items");
		return;
	}

	freelist_push(&b->freelist, el);

	// note: checking if <el> was actually allocated from <b> is difficult:
	// it may not be in the currently open bucket, so we'd have to
	// iterate over the list - too much work.
}


//-----------------------------------------------------------------------------
// matrix allocator
//-----------------------------------------------------------------------------

void** matrix_alloc(uint cols, uint rows, size_t el_size)
{
	const size_t initial_align = 64;
	// note: no provision for padding rows. this is a bit more work and
	// if el_size isn't a power-of-2, performance is going to suck anyway.
	// otherwise, the initial alignment will take care of it.

	const size_t ptr_array_size = cols*sizeof(void*);
	const size_t row_size = cols*el_size;
	const size_t data_size = rows*row_size;
	const size_t total_size = ptr_array_size + initial_align + data_size;

	void* p = malloc(total_size);
	if(!p)
		return 0;

	uintptr_t data_addr = (uintptr_t)p + ptr_array_size + initial_align;
	data_addr -= data_addr % initial_align;

	// alignment check didn't set address to before allocation
	debug_assert(data_addr >= (uintptr_t)p+ptr_array_size);

	void** ptr_array = (void**)p;
	for(uint i = 0; i < cols; i++)
	{
		ptr_array[i] = (void*)data_addr;
		data_addr += row_size;
	}

	// didn't overrun total allocation
	debug_assert(data_addr <= (uintptr_t)p+total_size);

	return ptr_array;
}


void matrix_free(void** matrix)
{
	free(matrix);
}


//-----------------------------------------------------------------------------
// allocator optimized for single instances
//-----------------------------------------------------------------------------

void* single_calloc(void* storage, volatile uintptr_t* in_use_flag, size_t size)
{
	// sanity check
	debug_assert(*in_use_flag == 0 || *in_use_flag == 1);

	void* p;

	// successfully reserved the single instance
	if(cpu_CAS(in_use_flag, 0, 1))
		p = storage;
	// already in use (rare) - allocate from heap
	else
		p = new u8[size];

	memset(p, 0, size);
	return p;
}


void single_free(void* storage, volatile uintptr_t* in_use_flag, void* p)
{
	// sanity check
	debug_assert(*in_use_flag == 0 || *in_use_flag == 1);

	if(p == storage)
	{
		if(cpu_CAS(in_use_flag, 1, 0))
		{
			// ok, flag has been reset to 0
		}
		else
			debug_warn("in_use_flag out of sync (double free?)");
	}
	// was allocated from heap
	else
	{
		// single instance may have been freed by now - cannot assume
		// anything about in_use_flag.

		delete[] (u8*)p;
	}
}


//-----------------------------------------------------------------------------
// static allocator
//-----------------------------------------------------------------------------

void* static_calloc(StaticStorage* ss, size_t size)
{
	void* p = (void*)round_up((uintptr_t)ss->pos, 16);
	ss->pos = (u8*)p+size;
	debug_assert(ss->pos <= ss->end);
	return p;
}
