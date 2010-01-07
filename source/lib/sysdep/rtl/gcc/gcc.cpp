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

#include "precompiled.h"

#include "lib/sysdep/rtl.h"
#include "lib/bits.h"

// Linux has posix_memalign, AFAIK our other gcc platforms do not have it.
#define HAVE_POSIX_MEMALIGN OS_LINUX

#if HAVE_POSIX_MEMALIGN

void* rtl_AllocateAligned(size_t size, size_t alignment)
{
	void *ptr;
	int ret = posix_memalign(&ptr, alignment, size);
	if (ret) {
		// TODO: report error?
		return NULL;
	}
	return ptr;
}

void rtl_FreeAligned(void* alignedPointer)
{
	free(alignedPointer);
}

#else // Fallback aligned allocation using malloc

void* rtl_AllocateAligned(size_t size, size_t align)
{
	// This ensures we have enough extra space to store the original pointer,
	// and produce an aligned buffer, assuming the platform malloc ensures at
	// least sizeof(void*) alignment.
	if (align < 2*sizeof(void*))
		align = 2*sizeof(void*);
	
	void* const malloc_ptr = malloc(size + align);
	if (!malloc_ptr)
		return NULL;
	
	// Round malloc_ptr up to the next aligned address, leaving some unused
	// space before the pointer we'll return. The minimum alignment above
	// ensures we'll have at least sizeof(void*) extra space.
	void* const aligned_ptr =
		(void *)(round_down(uintptr_t(malloc_ptr), uintptr_t(align)) + align);

	// Just make sure we did the right thing with all the alignment hacks above.
	debug_assert(((void**)aligned_ptr) - 1 >= malloc_ptr);

	// Store the original pointer which will have to be sent to free().
	((void **)aligned_ptr)[-1] = malloc_ptr;

	return aligned_ptr;
}

void rtl_FreeAligned(void* alignedPointer)
{
	if (alignedPointer)
		free(((void**)alignedPointer)[-1]);
}

#endif
