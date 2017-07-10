/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ALLOCATORS_ALLOCATOR_CHECKER
#define INCLUDED_ALLOCATORS_ALLOCATOR_CHECKER

#include <map>

/**
 * allocator test rig.
 * call from each allocator operation to sanity-check them.
 * should only be used during debug mode due to serious overhead.
 **/
class AllocatorChecker
{
public:
	void OnAllocate(void* p, size_t size)
	{
		const Allocs::value_type item = std::make_pair(p, size);
		std::pair<Allocs::iterator, bool> ret = allocs.insert(item);
		ENSURE(ret.second == true);	// wasn't already in map
	}

	void OnDeallocate(void* p, size_t size)
	{
		Allocs::iterator it = allocs.find(p);
		if(it == allocs.end())
			DEBUG_WARN_ERR(ERR::LOGIC);	// freeing invalid pointer
		else
		{
			// size must match what was passed to OnAllocate
			const size_t allocated_size = it->second;
			ENSURE(size == allocated_size);

			allocs.erase(it);
		}
	}

	/**
	 * allocator is resetting itself, i.e. wiping out all allocs.
	 **/
	void OnClear()
	{
		allocs.clear();
	}

private:
	typedef std::map<void*, size_t> Allocs;
	Allocs allocs;
};

#endif	// #ifndef INCLUDED_ALLOCATORS_ALLOCATOR_CHECKER
