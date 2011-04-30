/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * bucket allocator
 */

#ifndef INCLUDED_ALLOCATORS_BUCKET
#define INCLUDED_ALLOCATORS_BUCKET

/**
 * allocator design goals:
 * - either fixed- or variable-sized blocks;
 * - allow freeing individual blocks if they are all fixed-size;
 * - never relocates;
 * - no fixed limit.
 *
 * note: this type of allocator is called "region-based" in the literature
 * and is also known as "obstack"; see "Reconsidering Custom Memory
 * Allocation" (Berger, Zorn, McKinley).
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

	size_t el_size;

	/**
	 * records # buckets allocated; verifies the list of buckets is correct.
	 **/
	size_t num_buckets;
};


/**
 * ready the Bucket object for use.
 *
 * @param b Bucket*
 * @param el_size 0 to allow variable-sized allocations (which cannot be
 * freed individually); otherwise, it specifies the number of bytes that
 * will be returned by bucket_alloc (whose size parameter is then ignored).
 * @return LibError.
 **/
LIB_API LibError bucket_create(Bucket* b, size_t el_size);

/**
 * free all memory that ensued from \<b\>.
 *
 * future alloc and free calls on this Bucket will fail.
 *
 * @param b Bucket*
 **/
LIB_API void bucket_destroy(Bucket* b);

/**
 * Dole out memory from the Bucket.
 * exhausts the freelist before returning new entries to improve locality.
 *
 * @param b Bucket*
 * @param size bytes to allocate; ignored if bucket_create's el_size was not 0.
 * @return allocated memory, or 0 if the Bucket would have to be expanded and
 * there isn't enough memory to do so.
 **/
LIB_API void* bucket_alloc(Bucket* b, size_t size);

LIB_API void* bucket_fast_alloc(Bucket* b);

/**
 * make an entry available for reuse in the given Bucket.
 *
 * this is not allowed if created for variable-size elements.
 * rationale: avoids having to pass el_size here and compare with size when
 * allocating; also prevents fragmentation and leaking memory.
 *
 * @param b Bucket*
 * @param el entry allocated via bucket_alloc.
 **/
LIB_API void bucket_free(Bucket* b, void* el);

#endif	// #ifndef INCLUDED_ALLOCATORS_BUCKET
