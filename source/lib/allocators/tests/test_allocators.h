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

#include "lib/allocators/dynarray.h"
#include "lib/byte_order.h"

class TestAllocators : public CxxTest::TestSuite 
{
public:
	void test_da()
	{
		DynArray da;

		// basic test of functionality (not really meaningful)
		TS_ASSERT_OK(da_alloc(&da, 1000));
		TS_ASSERT_OK(da_set_size(&da, 1000));
		TS_ASSERT_OK(da_set_prot(&da, PROT_NONE));
		TS_ASSERT_OK(da_free(&da));

		// test wrapping existing mem blocks for use with da_read
		u8 data[4] = { 0x12, 0x34, 0x56, 0x78 };
		TS_ASSERT_OK(da_wrap_fixed(&da, data, sizeof(data)));
		u8 buf[4];
		TS_ASSERT_OK(da_read(&da, buf, 4));
		TS_ASSERT_EQUALS(read_le32(buf), (u32)0x78563412);	// read correct value
		debug_SkipErrors(ERR::FAIL);
		TS_ASSERT(da_read(&da, buf, 1) < 0);		// no more data left
		TS_ASSERT_EQUALS((uint32_t)debug_StopSkippingErrors(), (uint32_t)1);
		TS_ASSERT_OK(da_free(&da));
	}
};
