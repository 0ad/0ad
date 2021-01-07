/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/allocators/DynamicArena.h"

class TestDynamicArena : public CxxTest::TestSuite
{
public:
	void test_allocate()
	{
		Allocators::DynamicArena<100> testArena;
		u8* p = static_cast<u8*>(testArena.allocate(10, nullptr, 1));
		TS_ASSERT(p != nullptr);
		void* p2 = testArena.allocate(10, nullptr, 1);
		TS_ASSERT(p + 10 == p2);

		void* p3 = testArena.allocate(80, nullptr, 1);
		TS_ASSERT(p + 20 == p3);

		void* p4 = testArena.allocate(100, nullptr, 1);
		TS_ASSERT(p4 != nullptr);

		void* p5 = testArena.allocate(1, nullptr, 1);
		TS_ASSERT(p5 != nullptr);

		void* p6 = testArena.allocate(100, nullptr, 1);
		TS_ASSERT(p6 != nullptr);

		void* p7 = testArena.allocate(0, nullptr, 1);
		TS_ASSERT(p7 != nullptr);
	}

	void test_alignment()
	{
		Allocators::DynamicArena<100> testArena;
		u8* p = static_cast<u8*>(testArena.allocate(4, nullptr, 1));
		TS_ASSERT(p != nullptr);

		u8* p2 = static_cast<u8*>(testArena.allocate(1, nullptr, 8));
		TS_ASSERT_EQUALS(p + 8, p2);

		p2 = static_cast<u8*>(testArena.allocate(1, nullptr, 8));
		TS_ASSERT_EQUALS(p + 16, p2);

		p2 = static_cast<u8*>(testArena.allocate(1, nullptr, 8));
		TS_ASSERT_EQUALS(p + 24, p2);

		p2 = static_cast<u8*>(testArena.allocate(1, nullptr, 2));
		TS_ASSERT_EQUALS(p + 26, p2);

		p2 = static_cast<u8*>(testArena.allocate(1, nullptr, 8));
		TS_ASSERT_EQUALS(p + 32, p2);
	}
};
