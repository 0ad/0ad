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
 * File        : headerless.h
 * Project     : 0 A.D.
 * Description : (header-less) pool-based heap allocator
 * =========================================================================
 */

#ifndef INCLUDED_HEADERLESS
#define INCLUDED_HEADERLESS

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
	 * @param size [bytes] must be a multiple of the minimum alignment and
	 * enough to store a block header. (this allocator is designed for
	 * page-aligned requests but can handle smaller amounts.)
	 * @return allocated memory or 0 if the pool is too fragmented or full.
	 **/
	void* Allocate(size_t size) throw();

	/**
	 * deallocate memory.
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

#endif	// #ifndef INCLUDED_HEADERLESS
