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

#include "lib/self_test.h"

#include "lib/bits.h"	// round_down
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
		TS_ASSERT_EQUALS(a.Allocate(HeaderlessAllocator::minAllocationSize+1), null);

		// can't Allocate too small amounts
		TS_ASSERT_EQUALS(a.Allocate(HeaderlessAllocator::minAllocationSize/2), null);
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
		// implementation only allows sizes that are multiples of 0x10)
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
	void test_Randomized()
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
				const size_t size = std::max((size_t)HeaderlessAllocator::minAllocationSize, round_down(maxSize, HeaderlessAllocator::allocationAlignment));
				// (the size_t cast on minAllocationSize prevents max taking a reference to the non-defined variable)
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
