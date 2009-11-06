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

#include "lib/self_test.h"

#include "lib/allocators/headerless.h"

void* const null = 0;

class TestHeaderless: public CxxTest::TestSuite 
{
public:
	void test_Basic()
	{
		HeaderlessAllocator a(8192);

		// (these are disabled because they raise an assert)
#if 0
		// can't Allocate unaligned sizes
		TS_ASSERT_EQUALS(a.Allocate(1), null);

		// can't Allocate too small amounts
		TS_ASSERT_EQUALS(a.Allocate(16), null);
#endif

		// can Allocate the entire pool
		char* p1 = (char*)a.Allocate(4096);
		char* p2 = (char*)a.Allocate(4096);
		TS_ASSERT_DIFFERS(p1, null);
		TS_ASSERT_DIFFERS(p2, null);

		// back-to-back (non-freelist) allocations should be contiguous
		TS_ASSERT_EQUALS(p1+4096, p2);

		// allocations are writable
		p1[0] = 11;
		p1[4095] = 12;
	}

	void test_Free()
	{
		// Deallocate allows immediate reuse of the freed pointer
		HeaderlessAllocator a(4096);
		void* p1 = a.Allocate(1024);
		a.Deallocate(p1, 1024);
		void* p2 = a.Allocate(1024);
		TS_ASSERT_EQUALS(p1, p2);
	}

	void test_Coalesce()
	{
		HeaderlessAllocator a(0x10000);

		// can Allocate non-power-of-two sizes (note: the current
		// implementation only allows sizes that are multiples of 0x20)
		void* p1 = a.Allocate(0x5680);
		void* p2 = a.Allocate(0x78A0);
		void* p3 = a.Allocate(0x1240);
		TS_ASSERT_DIFFERS(p1, null);
		TS_ASSERT_DIFFERS(p2, null);
		TS_ASSERT_DIFFERS(p3, null);

		// after freeing, must be able to allocate the total amount
		// note: we don't insist on being able to fill the entire
		// memory range. in this case, the problem is that the pool has some
		// virtual address space left, but the allocator doesn't grab that
		// and add it to the freelist. that feature is currently not
		// implemented.
		a.Deallocate(p1, 0x5680);
		a.Deallocate(p2, 0x78A0);
		a.Deallocate(p3, 0x1240);
		void* p4 = a.Allocate(0xE140);
		TS_ASSERT_DIFFERS(p4, null);
	}

	void test_Reset()
	{
		// after Reset, must return the same pointer as a freshly constructed instance
		HeaderlessAllocator a(4096);
		void* p1 = a.Allocate(128);
		a.Reset();
		void* p2 = a.Allocate(128);
		TS_ASSERT_EQUALS(p1, p2);
	}

	// will the allocator survive a series of random but valid Allocate/Deallocate?
	void DISABLED_test_Randomized() // XXX: No it won't (on Linux/amd64)
	{
		const size_t poolSize = 1024*1024;
		HeaderlessAllocator a(poolSize);

		typedef std::map<void*, size_t> AllocMap;
		AllocMap allocs;

		srand(1);

		for(int i = 0; i < 1000; i++)
		{
			// allocate
			if(rand() >= RAND_MAX/2)
			{
				const size_t maxSize = (size_t)((rand() / (float)RAND_MAX) * poolSize);
				const size_t size = maxSize & ~0x1Fu;
				void* p = a.Allocate(size);
				if(!p)
					continue;
				TS_ASSERT(allocs.find(p) == allocs.end());
				allocs[p] = size;
			}
			// free
			else
			{
				if(allocs.empty())
					continue;
				// find random allocation to deallocate
				AllocMap::iterator it = allocs.begin();
				const int numToSkip = rand() % (int)allocs.size();
				for(int skip = 0; skip < numToSkip; skip++)
					++it;
				void* p = (*it).first;
				size_t size = (*it).second;
				allocs.erase(it);
				a.Deallocate(p, size);
			}
		}
	}
};
