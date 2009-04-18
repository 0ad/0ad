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

#include "lib/res/file/file_cache.h"
#include "lib/rand.h"

class TestFileCache : public CxxTest::TestSuite 
{
	enum { TEST_ALLOC_TOTAL = 100*1000*1000 };
public:
	void test_cache_allocator()
	{
		// allocated address -> its size
		typedef std::map<void*, size_t> AllocMap;
		AllocMap allocations;

		// put allocator through its paces by allocating several times
		// its capacity (this ensures memory is reused)
		srand(1);
		size_t total_size_used = 0;
		while(total_size_used < TEST_ALLOC_TOTAL)
		{
			size_t size = rand(1, TEST_ALLOC_TOTAL/16);
			total_size_used += size;
			void* p;
			// until successful alloc:
			for(;;)
			{
				p = file_cache_allocator_alloc(size);
				if(p)
					break;
				// out of room - remove a previous allocation
				// .. choose one at random
				size_t chosen_idx = (size_t)rand(0, (size_t)allocations.size());
				AllocMap::iterator it = allocations.begin();
				for(; chosen_idx != 0; chosen_idx--)
					++it;
				file_cache_allocator_free(it->first, it->second);
				allocations.erase(it);
			}

			// must not already have been allocated
			TS_ASSERT_EQUALS(allocations.find(p), allocations.end());
			allocations[p] = size;
		}

		// reset to virginal state
		// note: even though everything has now been freed, this is
		// necessary since the freelists may be a bit scattered already.
		file_cache_allocator_reset();
	}
};
