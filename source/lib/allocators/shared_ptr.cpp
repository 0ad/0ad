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

#include "precompiled.h"
#include "lib/allocators/shared_ptr.h"

#include "lib/allocators/allocator_checker.h"


#ifndef NDEBUG
static AllocatorChecker s_allocatorChecker;
#endif

class CheckedArrayDeleter
{
public:
	CheckedArrayDeleter(size_t size)
		: m_size(size)
	{
	}

	void operator()(u8* p)
	{
		ENSURE(m_size != 0);
#ifndef NDEBUG
		s_allocatorChecker.OnDeallocate(p, m_size);
#endif
		delete[] p;
		m_size = 0;
	}

private:
	size_t m_size;
};

shared_ptr<u8> Allocate(size_t size)
{
	ENSURE(size != 0);

	u8* p = new u8[size];
#ifndef NDEBUG
	s_allocatorChecker.OnAllocate(p, size);
#endif

	return shared_ptr<u8>(p, CheckedArrayDeleter(size));
}
