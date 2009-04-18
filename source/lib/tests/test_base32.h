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
		const u8 correct_out[] = "CJLYLIXZIHGVP4C";
		TS_ASSERT_SAME_DATA(out, correct_out, ARRAY_SIZE(correct_out));
	}
};
