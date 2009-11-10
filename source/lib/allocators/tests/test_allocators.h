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

#include "lib/allocators/allocators.h"
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
		TS_ASSERT_EQUALS(debug_StopSkippingErrors(), 1);
		TS_ASSERT_OK(da_free(&da));
	}

	void test_matrix()
	{
		// not much we can do here; allocate a matrix, write to it and
		// make sure it can be freed.
		// (note: can't check memory layout because "matrix" is int** -
		// array of pointers. the matrix interface doesn't guarantee
		// that data comes in row-major order after the row pointers)
		int** m = (int**)matrix_alloc(3, 3, sizeof(int));
		m[0][0] = 1;
		m[0][1] = 2;
		m[1][0] = 3;
		m[2][2] = 4;
		matrix_free((void**)m);
	}
};
