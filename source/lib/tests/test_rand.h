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
		TS_ASSERT_EQUALS(numSkipped, (size_t)2);
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
