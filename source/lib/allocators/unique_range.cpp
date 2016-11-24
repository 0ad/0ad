/* Copyright (c) 2015 Wildfire Games
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
#include "lib/allocators/unique_range.h"

#include "lib/bits.h"	// is_pow2, round_up
#include "lib/sysdep/cpu.h"	// cpu_AtomicAdd
#include "lib/sysdep/rtl.h"	// rtl_FreeAligned


static void FreeNone(void* UNUSED(pointer), size_t UNUSED(size))
{
	// (providing a deleter function for idxDeleterNone avoids
	// having to check whether deleters[idxDeleter] == 0)
}

static void FreeAligned(void* pointer, size_t UNUSED(size))
{
	return rtl_FreeAligned(pointer);
}


static UniqueRangeDeleter deleters[allocationAlignment] = { FreeNone, FreeAligned };

static IdxDeleter numDeleters = 2;


// NB: callers should skip this if *idxDeleterOut != 0 (avoids the overhead
// of an unnecessary indirect function call)
void RegisterUniqueRangeDeleter(UniqueRangeDeleter deleter, volatile IdxDeleter* idxDeleterOut)
{
	ENSURE(deleter);

	if(!cpu_CAS(idxDeleterOut, idxDeleterNone, -1))	// not the first call for this deleter
	{
		// wait until an index has been assigned
		while(*idxDeleterOut <= 0)
			cpu_Pause();
		return;
	}

	const IdxDeleter idxDeleter = cpu_AtomicAdd(&numDeleters, 1);
	ENSURE(idxDeleter < (IdxDeleter)ARRAY_SIZE(deleters));
	deleters[idxDeleter] = deleter;
	COMPILER_FENCE;
	*idxDeleterOut = idxDeleter;
}


NOTHROW_DEFINE void CallUniqueRangeDeleter(void* pointer, size_t size, IdxDeleter idxDeleter)
{
	ASSERT(idxDeleter < numDeleters);
	// (some deleters do not tolerate null pointers)
	if(pointer)
		deleters[idxDeleter](pointer, size);
}


UniqueRange AllocateAligned(size_t size, size_t alignment)
{
	ENSURE(is_pow2(alignment));
	alignment = std::max(alignment, allocationAlignment);

	const size_t alignedSize = round_up(size, alignment);
	const UniqueRange::pointer p = rtl_AllocateAligned(alignedSize, alignment);

	static volatile IdxDeleter idxDeleterAligned;
	if(idxDeleterAligned == 0)	// (optional optimization)
		RegisterUniqueRangeDeleter(FreeAligned, &idxDeleterAligned);

	return std::move(UniqueRange(p, size, idxDeleterAligned));
}


UniqueRange AllocateVM(size_t size, vm::PageType pageType, int prot)
{
	const UniqueRange::pointer p = vm::Allocate(size, pageType, prot);

	static volatile IdxDeleter idxDeleter;
	if(idxDeleter == 0)	// (optional optimization)
		RegisterUniqueRangeDeleter(vm::Free, &idxDeleter);

	return std::move(UniqueRange(p, size, idxDeleter));
}
