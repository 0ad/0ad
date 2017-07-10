/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ALLOCATORS_FREELIST
#define INCLUDED_ALLOCATORS_FREELIST

// "freelist" is a pointer to the first unused element or a sentinel.
// their memory holds a pointer to the previous element in the freelist
// (or its own address in the case of sentinels to avoid branches)
//
// rationale for the function-based interface: a class encapsulating the
// freelist pointer would force each header to include this header,
// whereas this approach only requires a void* pointer and calling
// mem_freelist_Sentinel from the implementation.
//
// these functions are inlined because allocation is sometimes time-critical.

// @return the address of a sentinel element, suitable for initializing
// a freelist pointer. subsequent mem_freelist_Detach on that freelist
// will return 0.
LIB_API void* mem_freelist_Sentinel();

static inline void mem_freelist_AddToFront(void*& freelist, void* el)
{
	ASSERT(freelist != 0);
	ASSERT(el != 0);

	memcpy(el, &freelist, sizeof(void*));
	freelist = el;
}

// @return 0 if the freelist is empty, else a pointer that had
// previously been passed to mem_freelist_AddToFront.
static inline void* mem_freelist_Detach(void*& freelist)
{
	ASSERT(freelist != 0);

	void* prev_el;
	memcpy(&prev_el, freelist, sizeof(void*));
	void* el = (freelist == prev_el)? 0 : freelist;
	freelist = prev_el;
	return el;
}

#endif	// #ifndef INCLUDED_ALLOCATORS_FREELIST
