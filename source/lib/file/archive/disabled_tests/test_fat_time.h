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
