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

#include "precompiled.h"

#include "lib/sysdep/rtl.h"
#include "lib/bits.h"

// Linux glibc has posix_memalign and (obsolete) memalign
// Android libc has only memalign
// OS X and BSD probably do not have either.
#define HAVE_POSIX_MEMALIGN (OS_LINUX && !OS_ANDROID)
#define HAVE_MEMALIGN OS_LINUX

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

#elif HAVE_MEMALIGN

void* rtl_AllocateAligned(size_t size, size_t alignment)
{
	return memalign(alignment, size);
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
	ENSURE(((void**)aligned_ptr) - 1 >= malloc_ptr);

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
