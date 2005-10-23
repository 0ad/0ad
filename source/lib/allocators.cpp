#include "precompiled.h"
#include "posix.h"

#include "allocators.h"


//-----------------------------------------------------------------------------
// expandable array
//-----------------------------------------------------------------------------

static const size_t page_size = sysconf(_SC_PAGE_SIZE);

static bool is_page_multiple(uintptr_t x)
{
	return (x % page_size) == 0;
}

static size_t round_up_to_page(size_t size)
{
	return round_up(size, page_size);
}

// indicates that this DynArray must not be resized or freed
// (e.g. because it merely wraps an existing memory range).
// stored in da->prot to reduce size; doesn't conflict with any PROT_* flags.
const int DA_NOT_OUR_MEM = 0x40000000;

static int validate_da(DynArray* da)
{
	if(!da)
		return ERR_INVALID_PARAM;
	u8* const base           = da->base;
	const size_t max_size_pa = da->max_size_pa;
	const size_t cur_size    = da->cur_size;
	const size_t pos         = da->pos;
	const int prot           = da->prot;

	if(debug_is_pointer_bogus(base))
		return -1;
	if(!is_page_multiple((uintptr_t)base))
		return -2;
	if(!is_page_multiple(max_size_pa))
		return -3;
	if(cur_size > max_size_pa)
		return -4;
	if(pos > cur_size || pos > max_size_pa)
		return -5;
	if(prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC|DA_NOT_OUR_MEM))
		return -6;

	return 0;
}

#define CHECK_DA(da) CHECK_ERR(validate_da(da))


//-----------------------------------------------------------------------------
// very thin wrapper on top of sys/mman.h that makes the intent more obvious
// (its commit/decommit semantics are difficult to tell apart).

static const int mmap_flags = MAP_PRIVATE|MAP_ANONYMOUS;

static int mem_reserve(size_t size, u8** pp)
{
	void* ret = mmap(0, size, PROT_NONE, mmap_flags|MAP_NORESERVE, -1, 0);
	if(ret == MAP_FAILED)
		return ERR_NO_MEM;
	*pp = (u8*)ret;
	return 0;
}

static int mem_release(u8* p, size_t size)
{
	return munmap(p, size);
}

static int mem_commit(u8* p, size_t size, int prot)
{
	if(prot == PROT_NONE)
	{
		debug_warn("mem_commit: prot=PROT_NONE isn't allowed (misinterpreted by mmap)");
		return ERR_INVALID_PARAM;
	}
	void* ret = mmap(p, size, prot, mmap_flags|MAP_FIXED, -1, 0);
	return (ret == MAP_FAILED)? -1 : 0;
}

static int mem_decommit(u8* p, size_t size)
{
	void* ret = mmap(p, size, PROT_NONE, mmap_flags|MAP_NORESERVE|MAP_FIXED, -1, 0);
	return (ret == MAP_FAILED)? -1 : 0;
}

static int mem_protect(u8* p, size_t size, int prot)
{
	return mprotect(p, size, prot);
}


//-----------------------------------------------------------------------------
// API

int da_alloc(DynArray* da, size_t max_size)
{
	const size_t max_size_pa = round_up_to_page(max_size);

	u8* p;
	CHECK_ERR(mem_reserve(max_size_pa, &p));

	da->base        = p;
	da->max_size_pa = max_size_pa;
	da->cur_size    = 0;
	da->pos         = 0;
	da->prot        = PROT_READ|PROT_WRITE;
	CHECK_DA(da);
	return 0;
}


int da_wrap_fixed(DynArray* da, u8* p, size_t size)
{
	da->base        = p;
	da->max_size_pa = round_up_to_page(size);
	da->cur_size    = size;
	da->pos         = 0;
	da->prot        = PROT_READ|PROT_WRITE|DA_NOT_OUR_MEM;
	CHECK_DA(da);
	return 0;
}


int da_free(DynArray* da)
{
	CHECK_DA(da);

	if(da->prot & DA_NOT_OUR_MEM)
	{
		debug_warn("da is marked DA_NOT_OUR_MEM, must not be altered");
		return -1;
	}

	// latch pointer; wipe out the DynArray for safety
	// (must be done here because mem_release may fail)
	u8* p = da->base;
	size_t size = da->max_size_pa;
	memset(da, 0, sizeof(*da));

	CHECK_ERR(mem_release(p, size));
	return 0;
}


int da_set_size(DynArray* da, size_t new_size)
{
	CHECK_DA(da);

	if(da->prot & DA_NOT_OUR_MEM)
	{
		debug_warn("da is marked DA_NOT_OUR_MEM, must not be altered");
		return -1;
	}

	// determine how much to add/remove
	const size_t cur_size_pa = round_up_to_page(da->cur_size);
	const size_t new_size_pa = round_up_to_page(new_size);
	if(new_size_pa > da->max_size_pa)
		CHECK_ERR(ERR_INVALID_PARAM);
	const ssize_t size_delta_pa = (ssize_t)new_size_pa - (ssize_t)cur_size_pa;

	u8* end = da->base + cur_size_pa;
	// expanding
	if(size_delta_pa > 0)
		CHECK_ERR(mem_commit(end, size_delta_pa, da->prot));
	// shrinking
	else if(size_delta_pa < 0)
		CHECK_ERR(mem_decommit(end+size_delta_pa, size_delta_pa));
	// else: no change in page count, e.g. if going from size=1 to 2
	// (we don't want mem_* to have to handle size=0)

	da->cur_size = new_size;
	CHECK_DA(da);
	return 0;
}


int da_set_prot(DynArray* da, int prot)
{
	CHECK_DA(da);

	// somewhat more subtle: POSIX mprotect requires the memory have been
	// mmap-ed, which it probably wasn't here.
	if(da->prot & DA_NOT_OUR_MEM)
	{
		debug_warn("da is marked DA_NOT_OUR_MEM, must not be altered");
		return -1;
	}

	da->prot = prot;
	CHECK_ERR(mem_protect(da->base, da->cur_size, prot));

	CHECK_DA(da);
	return 0;
}


int da_read(DynArray* da, void* data, size_t size)
{
	void* src = da->base + da->pos;

	// make sure we have enough data to read
	da->pos += size;
	if(da->pos > da->cur_size)
		return -1;

	memcpy2(data, src, size);
	return 0;
}


int da_append(DynArray* da, const void* data, size_t size)
{
	RETURN_ERR(da_set_size(da, da->pos+size));
	memcpy2(da->base+da->pos, data, size);
	da->pos += size;
	return 0;
}


//-----------------------------------------------------------------------------
// pool allocator
//-----------------------------------------------------------------------------

// design goals: O(1) alloc and free; doesn't preallocate the entire pool;
// returns sequential addresses.
//
// (note: this allocator returns fixed-size blocks, the size of which is
// specified at pool_create time. this makes O(1) time possible.) 

// parameters:
// - fixed-size allocations
// - can free/reuse allocations


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


static const size_t POOL_CHUNK = 4*KiB;


// ready <p> for use. pool_alloc will return chunks of memory that
// are exactly <el_size> bytes. <max_size> is the upper limit [bytes] on
// pool size (this is how much address space is reserved).
//
// note: el_size must at least be enough for a pointer (due to freelist
// implementation) but not exceed the expand-by amount.
int pool_create(Pool* p, size_t max_size, size_t el_size)
{
	if(el_size < sizeof(void*) || el_size > POOL_CHUNK)
		CHECK_ERR(ERR_INVALID_PARAM);

	RETURN_ERR(da_alloc(&p->da, max_size));
	p->pos = 0;
	p->el_size = el_size;
	return 0;
}


// free all memory that ensued from <p>. all elements are made unusable
// (it doesn't matter if they were "allocated" or in freelist or unused);
// future alloc and free calls on this pool will fail.
int pool_destroy(Pool* p)
{
	// don't be picky and complain if the freelist isn't empty;
	// we don't care since it's all part of the da anyway.
	// however, zero it to prevent further allocs from succeeding.
	p->freelist = 0;
	return da_free(&p->da);
}


// indicate whether <el> was allocated from the given pool.
// this is useful for callers that use several types of allocators.
bool pool_contains(Pool* p, void* el)
{
	// outside of our range
	if(!(p->da.base <= el && el < p->da.base+p->pos))
		return false;
	// sanity check: it should be aligned
	debug_assert((uintptr_t)((u8*)el - p->da.base) % p->el_size == 0);
	return true;
}


// return an entry from the pool, or 0 if it cannot be expanded as necessary.
// exhausts the freelist before returning new entries to improve locality.
void* pool_alloc(Pool* p)
{
	void* el = freelist_pop(&p->freelist);
	if(el)
		goto have_el;

	// alloc a new entry
	{
		// expand, if necessary
		if(p->pos + p->el_size > p->da.cur_size)
			if(da_set_size(&p->da, p->da.cur_size + POOL_CHUNK) < 0)
				return 0;

		el = p->da.base + p->pos;
		p->pos += p->el_size;
	}

have_el:
	debug_assert(pool_contains(p, el));	// paranoia
	return el;
}


// make <el> available for reuse in the given pool.
void pool_free(Pool* p, void* el)
{
	if(pool_contains(p, el))
		freelist_push(&p->freelist, el);
	else
		debug_warn("invalid pointer (not in pool)");
}


//-----------------------------------------------------------------------------
// bucket allocator
//-----------------------------------------------------------------------------

// parameters:
// - variable-size allocations
// - can only free all allocations at once
// - no init necessary
// - no fixed limit

// must be constant and power-of-2 to allow fast modulo.
const size_t BUCKET_SIZE = 4*KiB;


void* bucket_alloc(Bucket* b, size_t size)
{
	// would overflow a bucket
	if(size > BUCKET_SIZE-sizeof(u8*))
	{
		debug_warn("size doesn't fit in a bucket");
		return 0;
	}

	// make sure the next item will be aligned
	size = round_up(size, 8);

	// if there's not enough space left or no bucket yet (first call),
	// close it and allocate another.
	if(b->pos+size > BUCKET_SIZE || !b->bucket)
	{
		u8* bucket = (u8*)malloc(BUCKET_SIZE);
		if(!bucket)
			return 0;
		*(u8**)bucket = b->bucket;
		b->bucket = bucket;
		// skip bucket list field and align to 8 bytes (note: malloc already
		// aligns to at least 8 bytes, so don't take b->bucket into account)
		b->pos = round_up(sizeof(u8*), 8);
		b->num_buckets++;
	}

	void* ret = b->bucket+b->pos;
	b->pos += size;
	return ret;
}


void bucket_free_all(Bucket* b)
{
	while(b->bucket)
	{
		u8* prev_bucket = *(u8**)b->bucket;
		free(b->bucket);
		b->bucket = prev_bucket;
		b->num_buckets--;
	}

	debug_assert(b->num_buckets == 0);
}


//-----------------------------------------------------------------------------
// matrix allocator
//-----------------------------------------------------------------------------

// takes care of the dirty work of allocating 2D matrices:
// - aligns data
// - only allocates one memory block, which is more efficient than
//   malloc/new for each row.

// allocate a 2D cols x rows matrix of <el_size> byte cells.
// this must be freed via matrix_free. returns 0 if out of memory.
//
// the returned pointer should be cast to the target type (e.g. int**) and
// can then be accessed by matrix[col][row].
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


// free the given matrix (allocated by matrix_alloc). no-op if matrix == 0.
// callers will likely want to pass variables of a different type
// (e.g. int**); they must be cast to void**.
void matrix_free(void** matrix)
{
	free(matrix);
}


//-----------------------------------------------------------------------------
// built-in self test
//-----------------------------------------------------------------------------

#if SELF_TEST_ENABLED
namespace test {

static void test_api()
{
}

static void test_expand()
{
}

static void test_matrix()
{
	// not much we can do here; allocate a matrix and make sure
	// its memory layout is as expected (C-style row-major).
	int** m = (int**)matrix_alloc(3, 3, sizeof(int));
	m[0][0] = 1; debug_assert(((int*)m)[0*3+0] == 1);
	m[0][1] = 2; debug_assert(((int*)m)[0*3+1] == 2);
	m[1][0] = 3; debug_assert(((int*)m)[1*3+0] == 3);
	m[2][2] = 4; debug_assert(((int*)m)[2*3+2] == 4);
	matrix_free((void**)m);
}

static void self_test()
{
	test_api();
	test_expand();
	test_matrix();
}

RUN_SELF_TEST;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED
