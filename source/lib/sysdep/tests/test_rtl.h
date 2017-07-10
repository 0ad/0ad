/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/sysdep/rtl.h"

class Test_rtl : public CxxTest::TestSuite
{
	void _test_AllocateAligned_helper(size_t size, size_t align)
	{
		void* p = rtl_AllocateAligned(size, align);
		TS_ASSERT(p != NULL);
		TS_ASSERT_EQUALS((intptr_t)p % align, 0u);
		memset(p, 0x42, size);
		rtl_FreeAligned(p);
	}
public:
	void test_AllocateAligned()
	{
		for (size_t s = 0; s < 64; ++s)
		{
			_test_AllocateAligned_helper(s, 8);
			_test_AllocateAligned_helper(s, 16);
			_test_AllocateAligned_helper(s, 64);
			_test_AllocateAligned_helper(s, 1024);
			_test_AllocateAligned_helper(s, 65536);
		}
	}

	void test_FreeAligned_null()
	{
		rtl_FreeAligned(NULL);
	}
};
