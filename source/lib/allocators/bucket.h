/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : bucket.h
 * Project     : 0 A.D.
 * Description : bucket allocator
 * =========================================================================
 */

#ifndef INCLUDED_BUCKET
#define INCLUDED_BUCKET

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
 * @param Bucket*
 * @param el_size 0 to allow variable-sized allocations (which cannot be
 * freed individually); otherwise, it specifies the number of bytes that
 * will be returned by bucket_alloc (whose size parameter is then ignored).
 * @return LibError.
 **/
LIB_API LibError bucket_create(Bucket* b, size_t el_size);

/**
 * free all memory that ensued from <b>.
 *
 * future alloc and free calls on this Bucket will fail.
 *
 * @param Bucket*
 **/
LIB_API void bucket_destroy(Bucket* b);

/**
 * Dole out memory from the Bucket.
 * exhausts the freelist before returning new entries to improve locality.
 *
 * @param Bucket*
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
 * @param Bucket*
 * @param el entry allocated via bucket_alloc.
 **/
LIB_API void bucket_free(Bucket* b, void* el);

#endif	// #ifndef INCLUDED_BUCKET
