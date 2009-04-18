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

#include <ctime>

#include "lib/res/file/archive/fat_time.h"

class TestFatTime: public CxxTest::TestSuite 
{
public:
	void test_fat_timedate_conversion()
	{
		// note: FAT time stores second/2, which means converting may
		// end up off by 1 second.

		time_t t, converted_t;

		t = time(0);
		converted_t = time_t_from_FAT(FAT_from_time_t(t));
		TS_ASSERT_DELTA(t, converted_t, 2);

		t++;
		converted_t = time_t_from_FAT(FAT_from_time_t(t));
		TS_ASSERT_DELTA(t, converted_t, 2);
	}
};
