/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_ALLOCATORS_UNIQUE_RANGE
#define INCLUDED_ALLOCATORS_UNIQUE_RANGE

#include "lib/lib_api.h"
#include "lib/alignment.h"	// allocationAlignment
#include "lib/sysdep/vm.h"

// we usually don't hold multiple references to allocations, so unique_ptr
// can be used instead of the more complex (ICC generated incorrect code on
// 2 occasions) and expensive shared_ptr.
// a custom deleter is required because allocators such as ReserveAddressSpace need to
// pass the size to their deleter. we want to mix pointers from various allocators, but
// unique_ptr's deleter is fixed at compile-time, so it would need to be general enough
// to handle all allocators.
// storing the size and a function pointer would be one such solution, with the added
// bonus of no longer requiring a complete type at the invocation of ~unique_ptr.
// however, this inflates the pointer size to 3 words. if only a few allocator types
// are needed, we can replace the function pointer with an index stashed into the
// lower bits of the pointer (safe because all allocations' addresses are multiples
// of allocationAlignment).
typedef intptr_t IdxDeleter;

// no-op deleter (use when returning part of an existing allocation)
static const IdxDeleter idxDeleterNone = 0;

typedef void (*UniqueRangeDeleter)(void* pointer, size_t size);

/**
 * register a deleter, returning its index within the table.
 *
 * @param deleter function pointer. must be uniquely associated with
 *   the idxDeleter storage location.
 * @param idxDeleter location where to store the next available index.
 *   if it is already non-zero, skip the call to this function to
 *   avoid overhead.
 *
 * thread-safe. idxDeleter is used for mutual exclusion between
 * multiple callers for the same deleter. concurrent registration of
 * different deleters is also safe due to atomic increments.
 *
 * halts the program if more than allocationAlignment deleters are
 * to be registered.
 **/
LIB_API void RegisterUniqueRangeDeleter(UniqueRangeDeleter deleter, volatile IdxDeleter* idxDeleter);

LIB_API NOTHROW_DECLARE void CallUniqueRangeDeleter(void* pointer, size_t size, IdxDeleter idxDeleter);


// unfortunately, unique_ptr allows constructing without a custom deleter. to ensure callers can
// rely upon pointers being associated with a size, we introduce a `UniqueRange' replacement.
// its interface is identical to unique_ptr except for the constructors, the addition of
// size() and the removal of operator bool (which avoids implicit casts to int).
class UniqueRange
{
public:
	typedef void* pointer;
	typedef void element_type;

	UniqueRange()
	{
		Clear();
	}

	UniqueRange(pointer p, size_t size, IdxDeleter deleter)
	{
		Set(p, size, deleter);
	}

	UniqueRange(UniqueRange&& rvalue)
	{
		Pilfer(rvalue);
	}

	UniqueRange& operator=(UniqueRange&& rvalue)
	{
		UniqueRange& lvalue = rvalue;
		if(this != &lvalue)
		{
			Delete();
			Pilfer(lvalue);
		}
		return *this;
	}

	~UniqueRange()
	{
		Delete();
	}

	pointer get() const
	{
		return pointer(address_ & ~(allocationAlignment-1));
	}

	IdxDeleter get_deleter() const
	{
		return IdxDeleter(address_ % allocationAlignment);
	}

	size_t size() const
	{
		return size_;
	}

	// side effect: subsequent get_deleter will return idxDeleterNone
	pointer release()	// relinquish ownership
	{
		pointer ret = get();
		Clear();
		return ret;
	}

	void reset()
	{
		Delete();
		Clear();
	}

	void reset(pointer p, size_t size, IdxDeleter deleter)
	{
		Delete();
		Set(p, size, deleter);
	}

	void swap(UniqueRange& rhs)
	{
		std::swap(address_, rhs.address_);
		std::swap(size_, rhs.size_);
	}

	// don't define construction and assignment from lvalue,
	// but the declarations must be accessible
	UniqueRange(const UniqueRange&);
	UniqueRange& operator=(const UniqueRange&);

private:
	void Set(pointer p, size_t size, IdxDeleter deleter)
	{
		ASSERT((uintptr_t(p) % allocationAlignment) == 0);
		ASSERT(size_t(deleter) < allocationAlignment);

		address_ = uintptr_t(p) | deleter;
		size_ = size;

		ASSERT(get() == p);
		ASSERT(get_deleter() == deleter);
		ASSERT(this->size() == size);
	}

	void Clear()
	{
		Set(0, 0, idxDeleterNone);
	}

	void Pilfer(UniqueRange& victim)
	{
		const size_t size = victim.size();
		const IdxDeleter idxDeleter = victim.get_deleter();
		pointer p = victim.release();
		Set(p, size, idxDeleter);
		victim.Clear();
	}

	void Delete()
	{
		CallUniqueRangeDeleter(get(), size(), get_deleter());
	}

	// (IdxDeleter is stored in the lower bits of address since size might not even be a multiple of 4.)
	uintptr_t address_;
	size_t size_;
};

namespace std {

static inline void swap(UniqueRange& p1, UniqueRange& p2)
{
	p1.swap(p2);
}

static inline void swap(UniqueRange&& p1, UniqueRange& p2)
{
	p2.swap(p1);
}

static inline void swap(UniqueRange& p1, UniqueRange&& p2)
{
	p1.swap(p2);
}

}

LIB_API UniqueRange AllocateAligned(size_t size, size_t alignment);

LIB_API UniqueRange AllocateVM(size_t size, vm::PageType pageSize = vm::kDefault, int prot = PROT_READ|PROT_WRITE);


#endif	// #ifndef INCLUDED_ALLOCATORS_UNIQUE_RANGE
