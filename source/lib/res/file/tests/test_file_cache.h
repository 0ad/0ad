#include <cxxtest/TestSuite.h>

#include "lib/lib.h"
#include "lib/res/file/file_cache.h"

class TestFileCache : public CxxTest::TestSuite 
{
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
		while(total_size_used < 4*MAX_CACHE_SIZE)
		{
			size_t size = rand(1, MAX_CACHE_SIZE/4);
			total_size_used += size;
			void* p;
			// until successful alloc:
			for(;;)
			{
				p = cache_allocator.alloc(size);
				if(p)
					break;
				// out of room - remove a previous allocation
				// .. choose one at random
				size_t chosen_idx = (size_t)rand(0, (uint)allocations.size());
				AllocMap::iterator it = allocations.begin();
				for(; chosen_idx != 0; chosen_idx--)
					++it;
				cache_allocator.dealloc((u8*)it->first, it->second);
				allocations.erase(it);
			}

			// must not already have been allocated
			TS_ASSERT_EQUALS(allocations.find(p), allocations.end());
			allocations[p] = size;
		}

		// reset to virginal state
		cache_allocator.reset();
	}

	void test_file_cache()
	{
		// we need a unique address for file_cache_add, but don't want to
		// actually put it in the atom_fn storage (permanently clutters it).
		// just increment this pointer (evil but works since it's not used).
	//	const char* atom_fn = (const char*)1;
		// give to file_cache
	//	file_cache_add((FileIOBuf)p, size, atom_fn++);

		file_cache_reset();
		TS_ASSERT(file_cache.empty());

		// note: even though everything has now been freed,
		// the freelists may be a bit scattered already.
	}
};
