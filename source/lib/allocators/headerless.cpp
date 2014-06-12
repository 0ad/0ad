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

#include "precompiled.h"
#include "lib/allocators/headerless.h"

#include "lib/bits.h"
#include "lib/allocators/pool.h"


static const bool performSanityChecks = true;

// shared by Impl::Allocate and FreedBlock::Validate
static bool IsValidSize(size_t size);


//-----------------------------------------------------------------------------

// this combines the boundary tags and link fields into one structure,
// which is safer than direct pointer arithmetic.
//
// it is written to freed memory, which is fine because IsValidSize ensures
// the allocations are large enough.
class FreedBlock
{
	friend class RangeList;	// manipulates link fields directly

public:
	// (required for RangeList::m_sentinel)
	FreedBlock()
	{
	}

	void Setup(uintptr_t id, size_t size)
	{
		m_magic = s_magic;
		m_size = size;
		m_id = id;
	}

	void Reset()
	{
		// clear all fields to prevent accidental reuse
		prev = next = 0;
		m_id = 0;
		m_size = ~size_t(0);
		m_magic = 0;
	}

	size_t Size() const
	{
		return m_size;
	}

	/**
	 * @return whether this appears to be a FreedBlock instance with the
	 * desired ID. for additional safety, also call Validate().
	 **/
	bool IsFreedBlock(uintptr_t id) const
	{
		if(m_id != id)
			return false;
		if(m_magic != s_magic)
			return false;
		return true;
	}

	/**
	 * warn if any invariant doesn't hold.
	 **/
	void Validate(uintptr_t id) const
	{
		if(!performSanityChecks) return;

		// note: RangeList::Validate implicitly checks the prev and next
		// fields by iterating over the list.

		// note: we can't check for prev != next because we're called for
		// footers as well, and they don't have valid pointers.

		ENSURE(IsValidSize(m_size));
		ENSURE(IsFreedBlock(id));
	}

private:
	// note: the magic and ID fields are stored at both ends of this
	// class to increase the chance of detecting memory corruption.
	static const u64 s_magic = 0xFF55AA00BBCCDDEEull;
	u64 m_magic;

	FreedBlock* prev;
	FreedBlock* next;

	// size [bytes] of the entire memory block, including header and footer
	size_t m_size;

	// this differentiates between headers and footers.
	uintptr_t m_id;
};


static bool IsValidSize(size_t size)
{
	// note: we disallow the questionable practice of zero-byte allocations
	// because they may be indicative of bugs.
	if(size < HeaderlessAllocator::minAllocationSize)
		return false;

	if(size % HeaderlessAllocator::allocationAlignment)
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// freelists
//-----------------------------------------------------------------------------

// policy: address-ordered good fit
// mechanism: segregated range lists of power-of-two size classes

struct AddressOrder
{
	static bool ShouldInsertBefore(FreedBlock* current, FreedBlock* successor)
	{
		return current < successor;
	}
};

// "range list" is a freelist of similarly-sized blocks.
class RangeList
{
public:
	RangeList()
	{
		Reset();
	}

	void Reset()
	{
		m_sentinel.prev = &m_sentinel;
		m_sentinel.next = &m_sentinel;
		m_freeBlocks = 0;
		m_freeBytes = 0;
	}

	template<class InsertPolicy>
	void Insert(FreedBlock* freedBlock)
	{
		// find freedBlock before which to insert
		FreedBlock* successor;
		for(successor = m_sentinel.next; successor != &m_sentinel; successor = successor->next)
		{
			if(InsertPolicy::ShouldInsertBefore(freedBlock, successor))
				break;
		}

		freedBlock->prev = successor->prev;
		freedBlock->next = successor;
		successor->prev->next = freedBlock;
		successor->prev = freedBlock;

		m_freeBlocks++;
		m_freeBytes += freedBlock->Size();
	}

	/**
	 * @return the first freed block of size >= minSize or 0 if none exists.
	 **/
	FreedBlock* Find(size_t minSize)
	{
		for(FreedBlock* freedBlock = m_sentinel.next; freedBlock != &m_sentinel; freedBlock = freedBlock->next)
		{
			if(freedBlock->Size() >= minSize)
				return freedBlock;
		}

		// none found, so average block size is less than the desired size
		ENSURE(m_freeBytes/m_freeBlocks < minSize);
		return 0;
	}

	void Remove(FreedBlock* freedBlock)
	{
		freedBlock->next->prev = freedBlock->prev;
		freedBlock->prev->next = freedBlock->next;

		ENSURE(m_freeBlocks != 0);
		ENSURE(m_freeBytes >= freedBlock->Size());
		m_freeBlocks--;
		m_freeBytes -= freedBlock->Size();
	}

	void Validate(uintptr_t id) const
	{
		if(!performSanityChecks) return;

		size_t freeBlocks = 0, freeBytes = 0;

		for(FreedBlock* freedBlock = m_sentinel.next; freedBlock != &m_sentinel; freedBlock = freedBlock->next)
		{
			freedBlock->Validate(id);
			freeBlocks++;
			freeBytes += freedBlock->Size();
		}

		for(FreedBlock* freedBlock = m_sentinel.prev; freedBlock != &m_sentinel; freedBlock = freedBlock->prev)
		{
			freedBlock->Validate(id);
			freeBlocks++;
			freeBytes += freedBlock->Size();
		}

		// our idea of the number and size of free blocks is correct
		ENSURE(freeBlocks == m_freeBlocks*2 && freeBytes == m_freeBytes*2);
		// if empty, state must be as established by Reset
		ENSURE(!IsEmpty() || (m_sentinel.next == &m_sentinel && m_sentinel.prev == &m_sentinel));
	}

	bool IsEmpty() const
	{
		return (m_freeBlocks == 0);
	}

	size_t FreeBlocks() const
	{
		return m_freeBlocks;
	}

	size_t FreeBytes() const
	{
		return m_freeBytes;
	}

private:
	// a sentinel simplifies Insert and Remove. we store it here instead of
	// in a separate array to improve locality (it is actually accessed).
	mutable FreedBlock m_sentinel;

	size_t m_freeBlocks;
	size_t m_freeBytes;
};


//-----------------------------------------------------------------------------

class SegregatedRangeLists
{
public:
	SegregatedRangeLists()
	{
		Reset();
	}

	void Reset()
	{
		m_bitmap = 0;
		for(size_t i = 0; i < numRangeLists; i++)
			m_rangeLists[i].Reset();
	}

	void Insert(FreedBlock* freedBlock)
	{
		const size_t sizeClass = SizeClass(freedBlock->Size());
		m_rangeLists[sizeClass].Insert<AddressOrder>(freedBlock);

		m_bitmap |= Bit<uintptr_t>(sizeClass);
	}

	/**
	 * @return the first freed block of size >= minSize or 0 if none exists.
	 **/
	FreedBlock* Find(size_t minSize)
	{
		// iterate over all large enough, non-empty size classes
		// (zero overhead for empty size classes)
		const size_t minSizeClass = SizeClass(minSize);
		size_t sizeClassBits = m_bitmap & ((~size_t(0)) << minSizeClass);
		while(sizeClassBits)
		{
			const size_t size = ValueOfLeastSignificantOneBit(sizeClassBits);
			sizeClassBits &= ~size;	// remove from sizeClassBits
			const size_t sizeClass = SizeClass(size);

			FreedBlock* freedBlock = m_rangeLists[sizeClass].Find(minSize);
			if(freedBlock)
				return freedBlock;
		}

		// apparently all classes above minSizeClass are empty,
		// or the above would have succeeded.
		ENSURE(m_bitmap < Bit<uintptr_t>(minSizeClass+1));
		return 0;
	}

	void Remove(FreedBlock* freedBlock)
	{
		const size_t sizeClass = SizeClass(freedBlock->Size());
		m_rangeLists[sizeClass].Remove(freedBlock);

		// (masking with !IsEmpty() << sizeClass would probably be faster)
		if(m_rangeLists[sizeClass].IsEmpty())
			m_bitmap &= ~Bit<uintptr_t>(sizeClass);
	}

	void Validate(uintptr_t id) const
	{
		for(size_t i = 0; i < numRangeLists; i++)
		{
			m_rangeLists[i].Validate(id);

			// both bitmap and list must agree on whether they are empty
			ENSURE(((m_bitmap & Bit<uintptr_t>(i)) == 0) == m_rangeLists[i].IsEmpty());
		}
	}

	size_t FreeBlocks() const
	{
		size_t freeBlocks = 0;
		for(size_t i = 0; i < numRangeLists; i++)
			freeBlocks += m_rangeLists[i].FreeBlocks();
		return freeBlocks;
	}

	size_t FreeBytes() const
	{
		size_t freeBytes = 0;
		for(size_t i = 0; i < numRangeLists; i++)
			freeBytes += m_rangeLists[i].FreeBytes();
		return freeBytes;
	}

private:
	/**
	 * @return "size class" of a given size.
	 * class i > 0 contains blocks of size (2**(i-1), 2**i].
	 **/
	static size_t SizeClass(size_t size)
	{
		return ceil_log2(size);
	}

	static uintptr_t ValueOfLeastSignificantOneBit(uintptr_t x)
	{
		return (x & -(intptr_t)x);
	}

	// segregated, i.e. one list per size class.
	static const size_t numRangeLists = sizeof(uintptr_t)*CHAR_BIT;
	RangeList m_rangeLists[numRangeLists];

	// bit i set <==> size class i's freelist is not empty.
	// this allows finding a non-empty list in O(1).
	uintptr_t m_bitmap;
};


//-----------------------------------------------------------------------------
// coalescing
//-----------------------------------------------------------------------------

// policy: immediately coalesce
// mechanism: boundary tags

// note: the id and magic values are all that differentiates tags from
// user data. this isn't 100% reliable, but as with headers, we don't want
// to insert extra boundary tags into the allocated memory.

// note: footers consist of Tag{magic, ID, size}, while headers also
// need prev/next pointers. this could comfortably fit in 64 bytes,
// but we don't want to inherit headers from a base class because its
// prev/next pointers should reside between the magic and ID fields.
// maintaining separate FreedBlock and Footer classes is also undesirable;
// we prefer to use FreedBlock for both, which increases the minimum
// allocation size to 64 + allocationAlignment, e.g. 128.
// that's not a problem because the allocator is designed for
// returning pages or IO buffers (4..256 KB).
cassert(HeaderlessAllocator::minAllocationSize >= 2*sizeof(FreedBlock));


class BoundaryTagManager
{
public:
	BoundaryTagManager()
		: m_freeBlocks(0), m_freeBytes(0)
	{
	}

	FreedBlock* WriteTags(u8* p, size_t size)
	{
		FreedBlock* freedBlock = (FreedBlock*)p;
		freedBlock->Setup(s_headerId, size);
		Footer(freedBlock)->Setup(s_footerId, size);

		m_freeBlocks++;
		m_freeBytes += size;

		Validate(freedBlock);
		return freedBlock;
	}

	void RemoveTags(FreedBlock* freedBlock)
	{
		Validate(freedBlock);

		ENSURE(m_freeBlocks != 0);
		ENSURE(m_freeBytes >= freedBlock->Size());
		m_freeBlocks--;
		m_freeBytes -= freedBlock->Size();

		FreedBlock* footer = Footer(freedBlock);
		freedBlock->Reset();
		footer->Reset();
	}

	FreedBlock* PrecedingBlock(u8* p, u8* beginningOfPool) const
	{
		if(p == beginningOfPool)	// avoid accessing invalid memory
			return 0;

		FreedBlock* precedingBlock;
		{
			FreedBlock* const footer = (FreedBlock*)(p - sizeof(FreedBlock));
			if(!footer->IsFreedBlock(s_footerId))
				return 0;
			footer->Validate(s_footerId);
			precedingBlock = (FreedBlock*)(p - footer->Size());
		}

		Validate(precedingBlock);
		return precedingBlock;
	}

	FreedBlock* FollowingBlock(u8* p, size_t size, u8* endOfPool) const
	{
		if(p+size == endOfPool)	// avoid accessing invalid memory
			return 0;

		FreedBlock* const followingBlock = (FreedBlock*)(p + size);
		if(!followingBlock->IsFreedBlock(s_headerId))
			return 0;

		Validate(followingBlock);
		return followingBlock;
	}

	size_t FreeBlocks() const
	{
		return m_freeBlocks;
	}

	size_t FreeBytes() const
	{
		return m_freeBytes;
	}

	// (generated via GUID)
	static const uintptr_t s_headerId = 0x111E8E6Fu;
	static const uintptr_t s_footerId = 0x4D745342u;

private:
	void Validate(FreedBlock* freedBlock) const
	{
		if(!performSanityChecks) return;

		// the existence of freedBlock means our bookkeeping better have
		// records of at least that much memory.
		ENSURE(m_freeBlocks != 0);
		ENSURE(m_freeBytes >= freedBlock->Size());

		freedBlock->Validate(s_headerId);
		Footer(freedBlock)->Validate(s_footerId);
	}

	static FreedBlock* Footer(FreedBlock* freedBlock)
	{
		u8* const p = (u8*)freedBlock;
		return (FreedBlock*)(p + freedBlock->Size() - sizeof(FreedBlock));
	}

	size_t m_freeBlocks;
	size_t m_freeBytes;
};


//-----------------------------------------------------------------------------
// stats
//-----------------------------------------------------------------------------

class Stats
{
public:
	void OnReset()
	{
		if(!performSanityChecks) return;

		m_totalAllocatedBlocks = m_totalAllocatedBytes = 0;
		m_totalDeallocatedBlocks = m_totalDeallocatedBytes = 0;
		m_currentExtantBlocks = m_currentExtantBytes = 0;
		m_currentFreeBlocks = m_currentFreeBytes = 0;
	}

	void OnAllocate(size_t size)
	{
		if(!performSanityChecks) return;

		m_totalAllocatedBlocks++;
		m_totalAllocatedBytes += size;

		m_currentExtantBlocks++;
		m_currentExtantBytes += size;
	}

	void OnDeallocate(size_t size)
	{
		if(!performSanityChecks) return;

		m_totalDeallocatedBlocks++;
		m_totalDeallocatedBytes += size;
		ENSURE(m_totalDeallocatedBlocks <= m_totalAllocatedBlocks);
		ENSURE(m_totalDeallocatedBytes <= m_totalAllocatedBytes);

		ENSURE(m_currentExtantBlocks != 0);
		ENSURE(m_currentExtantBytes >= size);
		m_currentExtantBlocks--;
		m_currentExtantBytes -= size;
	}

	void OnAddToFreelist(size_t size)
	{
		m_currentFreeBlocks++;
		m_currentFreeBytes += size;
	}

	void OnRemoveFromFreelist(size_t size)
	{
		if(!performSanityChecks) return;

		ENSURE(m_currentFreeBlocks != 0);
		ENSURE(m_currentFreeBytes >= size);
		m_currentFreeBlocks--;
		m_currentFreeBytes -= size;
	}

	void Validate() const
	{
		if(!performSanityChecks) return;

		ENSURE(m_totalDeallocatedBlocks <= m_totalAllocatedBlocks);
		ENSURE(m_totalDeallocatedBytes <= m_totalAllocatedBytes);

		ENSURE(m_currentExtantBlocks == m_totalAllocatedBlocks-m_totalDeallocatedBlocks);
		ENSURE(m_currentExtantBytes == m_totalAllocatedBytes-m_totalDeallocatedBytes);
	}

	size_t FreeBlocks() const
	{
		return m_currentFreeBlocks;
	}

	size_t FreeBytes() const
	{
		return m_currentFreeBytes;
	}

private:
	u64 m_totalAllocatedBlocks, m_totalAllocatedBytes;
	u64 m_totalDeallocatedBlocks, m_totalDeallocatedBytes;
	u64 m_currentExtantBlocks, m_currentExtantBytes;
	u64 m_currentFreeBlocks, m_currentFreeBytes;
};


//-----------------------------------------------------------------------------
// HeaderlessAllocator::Impl
//-----------------------------------------------------------------------------

static void AssertEqual(size_t x1, size_t x2, size_t x3)
{
	ENSURE(x1 == x2 && x2 == x3);
}

class HeaderlessAllocator::Impl
{
public:
	Impl(size_t poolSize)
	{
		(void)pool_create(&m_pool, poolSize, 0);

		Reset();
	}

	~Impl()
	{
		Validate();

		(void)pool_destroy(&m_pool);
	}

	void Reset()
	{
		pool_free_all(&m_pool);
		m_segregatedRangeLists.Reset();
		m_stats.OnReset();

		Validate();
	}

	NOTHROW_DEFINE void* Allocate(size_t size)
	{
		ENSURE(IsValidSize(size));
		Validate();

		void* p = TakeAndSplitFreeBlock(size);
		if(!p)
		{
			p = pool_alloc(&m_pool, size);
			if(!p)			// both failed; don't throw bad_alloc because
				return 0;	// this often happens with the file cache.
		}

		// (NB: we must not update the statistics if allocation failed)
		m_stats.OnAllocate(size);

		Validate();
		ENSURE((uintptr_t)p % allocationAlignment == 0);
		return p;
	}

	void Deallocate(u8* p, size_t size)
	{
		ENSURE((uintptr_t)p % allocationAlignment == 0);
		ENSURE(IsValidSize(size));
		ENSURE(pool_contains(&m_pool, p));
		ENSURE(pool_contains(&m_pool, p+size-1));

		Validate();

		m_stats.OnDeallocate(size);
		Coalesce(p, size);
		AddToFreelist(p, size);

		Validate();
	}

	void Validate() const
	{
		if(!performSanityChecks) return;

		m_segregatedRangeLists.Validate(BoundaryTagManager::s_headerId);
		m_stats.Validate();

		AssertEqual(m_stats.FreeBlocks(), m_segregatedRangeLists.FreeBlocks(), m_boundaryTagManager.FreeBlocks());
		AssertEqual(m_stats.FreeBytes(), m_segregatedRangeLists.FreeBytes(), m_boundaryTagManager.FreeBytes());
	}

private:
	void AddToFreelist(u8* p, size_t size)
	{
		FreedBlock* freedBlock = m_boundaryTagManager.WriteTags(p, size);
		m_segregatedRangeLists.Insert(freedBlock);
		m_stats.OnAddToFreelist(size);
	}

	void RemoveFromFreelist(FreedBlock* freedBlock)
	{
		m_stats.OnRemoveFromFreelist(freedBlock->Size());
		m_segregatedRangeLists.Remove(freedBlock);
		m_boundaryTagManager.RemoveTags(freedBlock);
	}

	/**
	 * expand a block by coalescing it with its free neighbor(s).
	 **/
	void Coalesce(u8*& p, size_t& size)
	{
		{
			FreedBlock* precedingBlock = m_boundaryTagManager.PrecedingBlock(p, m_pool.da.base);
			if(precedingBlock)
			{
				p -= precedingBlock->Size();
				size += precedingBlock->Size();
				RemoveFromFreelist(precedingBlock);
			}
		}

		{
			FreedBlock* followingBlock = m_boundaryTagManager.FollowingBlock(p, size, m_pool.da.base+m_pool.da.pos);
			if(followingBlock)
			{
				size += followingBlock->Size();
				RemoveFromFreelist(followingBlock);
			}
		}
	}

	void* TakeAndSplitFreeBlock(size_t size)
	{
		u8* p;
		size_t leftoverSize = 0;
		{
			FreedBlock* freedBlock = m_segregatedRangeLists.Find(size);
			if(!freedBlock)
				return 0;

			p = (u8*)freedBlock;
			leftoverSize = freedBlock->Size() - size;
			RemoveFromFreelist(freedBlock);
		}

		if(IsValidSize(leftoverSize))
			AddToFreelist(p+size, leftoverSize);

		return p;
	}

	Pool m_pool;
	SegregatedRangeLists m_segregatedRangeLists;
	BoundaryTagManager m_boundaryTagManager;
	Stats m_stats;
};


//-----------------------------------------------------------------------------

HeaderlessAllocator::HeaderlessAllocator(size_t poolSize)
	: impl(new Impl(poolSize))
{
}

void HeaderlessAllocator::Reset()
{
	return impl->Reset();
}

NOTHROW_DEFINE void* HeaderlessAllocator::Allocate(size_t size)
{
	return impl->Allocate(size);
}

void HeaderlessAllocator::Deallocate(void* p, size_t size)
{
	return impl->Deallocate((u8*)p, size);
}

void HeaderlessAllocator::Validate() const
{
	return impl->Validate();
}
