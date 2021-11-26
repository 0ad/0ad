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

#include "lib/self_test.h"
#include "lib/res/file/archive/compression.h"

#include <random>

class TestCompression : public CxxTest::TestSuite
{
public:
	void test_compress_decompress_compare()
	{
		// generate random input data
		// (limit values to 0..7 so that the data will actually be compressible)
		std::mt19937 engine(42);
		std::uniform_int_distribution<u8> distribution(0x00, 0x07);
		const size_t data_size = 10000;
		u8 data[data_size];
		for(size_t i = 0; i < data_size; i++)
			data[i] = distribution(engine);

		u8* cdata; size_t csize;
		u8 udata[data_size];

		// compress
		uintptr_t c = comp_alloc(CT_COMPRESSION, CM_DEFLATE);
		{
		TS_ASSERT(c != 0);
		const size_t csizeBound = comp_max_output_size(c, data_size);
		TS_ASSERT_OK(comp_alloc_output(c, csizeBound));
		const ssize_t cdata_produced = comp_feed(c, data, data_size);
		TS_ASSERT(cdata_produced >= 0);
		u32 checksum;
		TS_ASSERT_OK(comp_finish(c, &cdata, &csize, &checksum));
		TS_ASSERT(cdata_produced <= (ssize_t)csize);	// can't have produced more than total
		}

		// decompress
		uintptr_t d = comp_alloc(CT_DECOMPRESSION, CM_DEFLATE);
		{
		TS_ASSERT(d != 0);
		comp_set_output(d, udata, data_size);
		const ssize_t udata_produced = comp_feed(d, cdata, csize);
		TS_ASSERT(udata_produced >= 0);
		u8* udata_final; size_t usize_final; u32 checksum;
		TS_ASSERT_OK(comp_finish(d, &udata_final, &usize_final, &checksum));
		TS_ASSERT(udata_produced <= (ssize_t)usize_final);	// can't have produced more than total
		TS_ASSERT_EQUALS(udata_final, udata);	// output buffer address is same
		TS_ASSERT_EQUALS(usize_final, data_size);	// correct amount of output
		}

		comp_free(c);
		comp_free(d);

		// verify data survived intact
		TS_ASSERT_SAME_DATA(data, udata, data_size);
	}
};
