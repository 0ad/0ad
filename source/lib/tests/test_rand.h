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

#include "lib/rand.h"

class TestRand : public CxxTest::TestSuite 
{
public:
	// complain if huge interval or min > max
	void TestParam()
	{
		debug_SkipErrors(ERR::INVALID_PARAM);
		TS_ASSERT_EQUALS(rand(1, 0), size_t(0));
		TS_ASSERT_EQUALS(rand(2, ~0u), size_t(0));
		const size_t numSkipped = debug_StopSkippingErrors();
		TS_ASSERT_EQUALS(numSkipped, 2);
	}

	// returned number must be in [min, max)
	void TestReturnedRange()
	{
		for(int i = 0; i < 100; i++)
		{
			size_t min = rand(), max = min+rand();
			size_t x = rand(min, max);
			TS_ASSERT(min <= x && x < max);
		}
	}

	// make sure both possible values are hit
	void TestTwoValues()
	{
		size_t ones = 0, twos = 0;
		for(int i = 0; i < 100; i++)
		{
			size_t x = rand(1, 3);
			// paranoia: don't use array (x might not be 1 or 2 - checked below)
			if(x == 1) ones++;
			if(x == 2) twos++;
		}
		TS_ASSERT_EQUALS(ones+twos, size_t(100));
		TS_ASSERT(ones > 10 && twos > 10);
	}
};
