#include "precompiled.h"
#include "lib/allocators/unique_range.h"

#include "lib/sysdep/cpu.h"	// cpu_AtomicAdd
#include "lib/sysdep/rtl.h"	// rtl_FreeAligned


static void UniqueRangeDeleterNone(void* UNUSED(pointer), size_t UNUSED(size))
{
	// (introducing this do-nothing function avoids having to check whether deleter != 0)
}

static void UniqueRangeDeleterAligned(void* pointer, size_t UNUSED(size))
{
	return rtl_FreeAligned(pointer);
}


static UniqueRangeDeleter deleters[idxDeleterBits+1] = { UniqueRangeDeleterNone, UniqueRangeDeleterAligned };

static IdxDeleter numDeleters = 2;


IdxDeleter AddUniqueRangeDeleter(UniqueRangeDeleter deleter)
{
	ENSURE(deleter);
	IdxDeleter idxDeleter = cpu_AtomicAdd(&numDeleters, 1);
	ENSURE(idxDeleter < (IdxDeleter)ARRAY_SIZE(deleters));
	deleters[idxDeleter] = deleter;
	return idxDeleter;
}


void CallUniqueRangeDeleter(void* pointer, size_t size, IdxDeleter idxDeleter) throw()
{
	ASSERT(idxDeleter < numDeleters);
	// (some deleters do not tolerate null pointers)
	if(pointer)
		deleters[idxDeleter](pointer, size);
}
