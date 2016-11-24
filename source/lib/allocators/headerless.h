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
 * (header-less) pool-based heap allocator
 */

#ifndef INCLUDED_ALLOCATORS_HEADERLESS
#define INCLUDED_ALLOCATORS_HEADERLESS

/**
 * (header-less) pool-based heap allocator
 * provides Allocate and Deallocate without requiring in-band headers;
 * this is useful when allocating page-aligned I/O buffers
 * (headers would waste an entire page per buffer)
 *
 * policy:
 * - allocation: first exhaust the freelist, then allocate more
 * - freelist: address-ordered good fit, always split blocks
 * - coalescing: immediate
 * mechanism:
 * - coalescing: boundary tags in freed memory with distinct bit patterns
 * - freelist: segregated range lists of power-of-two size classes
 *
 * note: this module basically implements a (rather complex) freelist and
 * could be made independent of the Pool allocation scheme. however, reading
 * neighboring boundary tags may cause segmentation violations; knowing the
 * bounds of valid committed memory (i.e. Pool extents) avoids this.
 **/
class HeaderlessAllocator
{
public:
	// allocators must 'naturally' align pointers, i.e. ensure they are
	// multiples of the largest native type (currently __m128).
	// since there are no headers, we can guarantee alignment by
	// requiring sizes to be multiples of allocationAlignment.
	static const size_t allocationAlignment = 16;

	// allocations must be large enough to hold our boundary tags
	// when freed. (see rationale above BoundaryTagManager)
	static const size_t minAllocationSize = 128;

	/**
	 * @param poolSize maximum amount of memory that can be allocated.
	 * this much virtual address space is reserved up-front (see Pool).
	 **/
	HeaderlessAllocator(size_t poolSize);

	/**
	 * restore the original state (as if newly constructed).
	 * this includes reclaiming all extant allocations.
	 **/
	void Reset();

	/**
	 * @param size [bytes] (= minAllocationSize + i*allocationAlignment).
	 * (this allocator is designed for requests on the order of several KiB)
	 * @return allocated memory or 0 if the pool is too fragmented or full.
	 **/
	NOTHROW_DECLARE void* Allocate(size_t size);

	/**
	 * deallocate memory.
	 * @param p must be exactly as returned by Allocate (in particular,
	 * evenly divisible by allocationAlignment)
	 * @param size must be exactly as specified to Allocate.
	 **/
	void Deallocate(void* p, size_t size);

	/**
	 * perform sanity checks; ensure allocator state is consistent.
	 **/
	void Validate() const;

private:
	class Impl;
	shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_ALLOCATORS_HEADERLESS
