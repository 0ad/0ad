/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "lib/self_test.h"
#include "lib/res/file/archive/compression.h"

class TestCompression : public CxxTest::TestSuite 
{
public:
	void test_compress_decompress_compare()
	{
		// generate random input data
		// (limit values to 0..7 so that the data will actually be compressible)
		const size_t data_size = 10000;
		u8 data[data_size];
		for(size_t i = 0; i < data_size; i++)
			data[i] = rand() & 0x07;

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
