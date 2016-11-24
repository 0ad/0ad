/* Copyright (c) 2011 Wildfire Games
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
 * arena allocator (variable-size blocks, no deallocation).
 */

#include "precompiled.h"
#include "lib/allocators/arena.h"

namespace Allocators {

template<class Storage>
struct BasicArenaTest
{
	void operator()() const
	{
		Arena<Storage> a(100);
		const size_t initialSpace = a.RemainingBytes();
		void* p = a.allocate(100);
		ENSURE(p != 0);
		ENSURE(a.Contains(uintptr_t(p)));
		ENSURE(a.RemainingBytes() == initialSpace-100);
		ENSURE(a.Contains(uintptr_t(p)+1));
		ENSURE(a.Contains(uintptr_t(p)+99));
		ENSURE(!a.Contains(uintptr_t(p)-1));
		ENSURE(!a.Contains(uintptr_t(p)+100));
		if(a.RemainingBytes() == 0)
			ENSURE(a.allocate(1) == 0);	// full
		else
			ENSURE(a.allocate(1) != 0);	// can still expand
		a.DeallocateAll();
		ENSURE(!a.Contains(uintptr_t(p)));

		p = a.allocate(36);
		ENSURE(p != 0);
		ENSURE(a.Contains(uintptr_t(p)));
		ENSURE(a.RemainingBytes() == initialSpace-36);
		void* p2 = a.allocate(64);
		ENSURE(p2 != 0);
		ENSURE(a.Contains(uintptr_t(p2)));
		ENSURE(a.RemainingBytes() == initialSpace-36-64);
		ENSURE(p2 == (void*)(uintptr_t(p)+36));
		if(a.RemainingBytes() == 0)
			ENSURE(a.allocate(1) == 0);	// full
		else
			ENSURE(a.allocate(1) != 0);	// can still expand
	}
};

void TestArena()
{
	ForEachStorage<BasicArenaTest>();
}

}	// namespace Allocators
