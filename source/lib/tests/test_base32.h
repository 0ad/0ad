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

#include "lib/base32.h"

class TestBase32 : public CxxTest::TestSuite
{
public:
	void test_base32()
	{
		// compare against previous output (generated via this base32() call)
		const u8 in[] = { 0x12, 0x57, 0x85, 0xA2, 0xF9, 0x41, 0xCD, 0x57, 0xF3 };
		u8 out[20] = {0};
		base32(ARRAY_SIZE(in), in, out);
		const u8 correct_out[] = "CJLYLIXZIHGVP4Y";
		TS_ASSERT_SAME_DATA(out, correct_out, ARRAY_SIZE(correct_out));
	}

	void test_base32_lengths()
	{
#define TEST(in, expected) \
		{ \
			u8 out[20] = {0}; \
			base32(ARRAY_SIZE(in), in, out); \
			const u8 correct_out[] = expected; \
			TS_ASSERT_SAME_DATA(out, correct_out, ARRAY_SIZE(correct_out)); \
		}
		const u8 in1[] = { 0xFF };
		const u8 in2[] = { 0xFF, 0xFF };
		const u8 in3[] = { 0xFF, 0xFF, 0xFF };
		const u8 in4[] = { 0xFF, 0xFF, 0xFF, 0xFF };
		const u8 in5[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		const u8 in6[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		TEST(in1, "74");
		TEST(in2, "777Q");
		TEST(in3, "77776");
		TEST(in4, "777777Y");
		TEST(in5, "77777777");
		TEST(in6, "7777777774");
	}
};
