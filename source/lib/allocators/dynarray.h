/**
 * =========================================================================
 * File        : dynarray.h
 * Project     : 0 A.D.
 * Description : dynamic (expandable) array
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_DYNARRAY
#define INCLUDED_DYNARRAY

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
LIB_API LibError da_alloc(DynArray* da, size_t max_size);

/**
 * free all memory (address space + physical) that constitutes the
 * given array.
 *
 * use-after-free is impossible because the memory is unmapped.
 *
 * @param DynArray* da; zeroed afterwards.
 * @return LibError
 **/
LIB_API LibError da_free(DynArray* da);

/**
 * expand or shrink the array: changes the amount of currently committed
 * (i.e. usable) memory pages.
 *
 * @param da DynArray.
 * @param new_size target size (rounded up to next page multiple).
 * pages are added/removed until this is met.
 * @return LibError.
 **/
LIB_API LibError da_set_size(DynArray* da, size_t new_size);

/**
 * Make sure at least <size> bytes starting at da->pos are committed and
 * ready for use.
 *
 * @param DynArray*
 * @param size Minimum amount to guarantee [bytes]
 * @return LibError
 **/
LIB_API LibError da_reserve(DynArray* da, size_t size);

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
LIB_API LibError da_set_prot(DynArray* da, int prot);

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
LIB_API LibError da_wrap_fixed(DynArray* da, u8* p, size_t size);

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
LIB_API LibError da_read(DynArray* da, void* data_dst, size_t size);

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
LIB_API LibError da_append(DynArray* da, const void* data_src, size_t size);

#endif	// #ifndef INCLUDED_DYNARRAY
