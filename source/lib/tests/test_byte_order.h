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

#include "lib/byte_order.h"

class TestByteOrder : public CxxTest::TestSuite
{
public:
	void test_conversion()
	{
		const u32 x = 0x01234567u;
		u8 LS_byte;
		memcpy(&LS_byte, &x, 1);
		// little endian
		if(LS_byte == 0x67)
		{
			TS_ASSERT_EQUALS(to_le16(0x0123u), 0x0123u);
			TS_ASSERT_EQUALS(to_le32(0x01234567u), 0x01234567u);
			TS_ASSERT_EQUALS(to_le64(0x0123456789ABCDEFull), 0x0123456789ABCDEFull);

			TS_ASSERT_EQUALS(to_be16(0x0123u), 0x2301u);
			TS_ASSERT_EQUALS(to_be32(0x01234567u), 0x67452301u);
			TS_ASSERT_EQUALS(to_be64(0x0123456789ABCDEFull), 0xEFCDAB8967452301ull);
		}
		// big endian
		else if(LS_byte == 0x01)
		{
			TS_ASSERT_EQUALS(to_le16(0x0123u), 0x2301u);
			TS_ASSERT_EQUALS(to_le32(0x01234567u), 0x67452301u);
			TS_ASSERT_EQUALS(to_le64(0x0123456789ABCDEFull), 0xEFCDAB8967452301ull);

			TS_ASSERT_EQUALS(to_be16(0x0123u), 0x0123u);
			TS_ASSERT_EQUALS(to_be32(0x01234567u), 0x01234567u);
			TS_ASSERT_EQUALS(to_be64(0x0123456789ABCDEFull), 0x0123456789ABCDEFull);
		}
		else
			TS_FAIL("endian determination failed");

		// note: no need to test read_?e* / write_?e* - they are
		// trivial wrappers on top of to_?e*.
	}

	void test_movzx()
	{
		const u8 d1[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
		const u8 d2[] = { 0x43, 0x12, 0x23, 0xA4 };

		TS_ASSERT_EQUALS(movzx_le64(d1, 1), 0x01ull);
		TS_ASSERT_EQUALS(movzx_le64(d1, 2), 0x0201ull);
		TS_ASSERT_EQUALS(movzx_le64(d1, 8), 0x0807060504030201ull);
		TS_ASSERT_EQUALS(movzx_le64(d2, 4), 0xA4231243ull);
		TS_ASSERT_EQUALS(movzx_le64(d2+3, 1), 0xA4ull);

		TS_ASSERT_EQUALS(movzx_be64(d1, 1), 0x01ull);
		TS_ASSERT_EQUALS(movzx_be64(d1, 2), 0x0102ull);
		TS_ASSERT_EQUALS(movzx_be64(d1, 8), 0x0102030405060708ull);
		TS_ASSERT_EQUALS(movzx_be64(d2, 4), 0x431223A4ull);
		TS_ASSERT_EQUALS(movzx_be64(d2+3, 1), 0xA4ull);
	}

	void test_movsx()
	{
		const u8 d1[] = { 0x09, 0xFE };
		const u8 d2[] = { 0xD9, 0x2C, 0xDD, 0x8F };
		const u8 d3[] = { 0x92, 0x26, 0x88, 0xF1, 0x35, 0xAC, 0x01, 0x83 };

		TS_ASSERT_EQUALS(movsx_le64(d1, 1), (i64)0x09ull);
		TS_ASSERT_EQUALS(movsx_le64(d1, 2), (i64)0xFFFFFFFFFFFFFE09ull);
		TS_ASSERT_EQUALS(movsx_le64(d2, 4), (i64)0xFFFFFFFF8FDD2CD9ull);
		TS_ASSERT_EQUALS(movsx_le64(d3, 8), (i64)0x8301AC35F1882692ull);

		TS_ASSERT_EQUALS(movsx_be64(d1, 1), (i64)0x09ull);
		TS_ASSERT_EQUALS(movsx_be64(d1, 2), (i64)0x00000000000009FEull);
		TS_ASSERT_EQUALS(movsx_be64(d2, 4), (i64)0xFFFFFFFFD92CDD8Full);
		TS_ASSERT_EQUALS(movsx_be64(d3, 8), (i64)0x922688F135AC0183ull);
	}
};
