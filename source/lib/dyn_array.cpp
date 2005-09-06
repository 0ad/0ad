#include "precompiled.h"
#include "posix.h"

#include "dyn_array.h"


static const size_t page_size = sysconf(_SC_PAGE_SIZE);

static bool is_page_multiple(uintptr_t x)
{
	return (x % page_size) == 0;
}

static size_t round_up_to_page(size_t size)
{
	return round_up(size, page_size);
}


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
	if(prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
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


int da_free(DynArray* da)
{
	CHECK_DA(da);

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

	memcpy(data, src, size);
	return 0;
}


int da_append(DynArray* da, const void* data, size_t size)
{
	RETURN_ERR(da_set_size(da, da->pos+size));
	memcpy(da->base+da->pos, data, size);
	da->pos += size;
	return 0;
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

static void self_test()
{
	test_api();
	test_expand();
}

RUN_SELF_TEST;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED