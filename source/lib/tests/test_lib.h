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

#include "lib/lib.h"

class TestLib : public CxxTest::TestSuite
{
public:
	void test_hi_lo()
	{
		TS_ASSERT_EQUALS(u64_hi(0x0123456789ABCDEFull), 0x01234567u);
		TS_ASSERT_EQUALS(u64_hi(0x0000000100000002ull), 0x00000001u);

		TS_ASSERT_EQUALS(u64_lo(0x0123456789ABCDEFull), 0x89ABCDEFu);
		TS_ASSERT_EQUALS(u64_lo(0x0000000100000002ull), 0x00000002u);

		TS_ASSERT_EQUALS(u32_hi(0x01234567u), 0x0123u);
		TS_ASSERT_EQUALS(u32_hi(0x00000001u), 0x0000u);

		TS_ASSERT_EQUALS(u32_lo(0x01234567u), 0x4567u);
		TS_ASSERT_EQUALS(u32_lo(0x00000001u), 0x0001u);

		TS_ASSERT_EQUALS(u64_from_u32(0xFFFFFFFFu, 0x80000008u), 0xFFFFFFFF80000008ull);
		TS_ASSERT_EQUALS(u32_from_u16(0x8000u, 0xFFFFu), 0x8000FFFFu);
	}

	// fp_to_u?? already validate the result.
};
