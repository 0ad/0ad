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

#include "lib/fnv_hash.h"

class TestFnvHash : public CxxTest::TestSuite
{
public:
	void test_fnv_hash()
	{
		TS_ASSERT_EQUALS(fnv_hash(""), 0x811C9DC5u);		// verify initial value
		const u32 h1 = fnv_hash("abcdef");
		TS_ASSERT_EQUALS(h1, 0xFF478A2A);					// verify value for simple string
		TS_ASSERT_EQUALS(fnv_hash   ("abcdef", 6), h1);	// same result if hashing buffer
		TS_ASSERT_EQUALS(fnv_lc_hash("ABcDeF", 6), h1);	// same result if case differs

		TS_ASSERT_EQUALS(fnv_hash64(""), 0xCBF29CE484222325ull);	// verify initial value
		const u64 h2 = fnv_hash64("abcdef");
		TS_ASSERT_EQUALS(h2, 0xD80BDA3FBE244A0Aull);		// verify value for simple string
		TS_ASSERT_EQUALS(fnv_hash64("abcdef", 6), h2);	// same result if hashing buffer
	}
};
