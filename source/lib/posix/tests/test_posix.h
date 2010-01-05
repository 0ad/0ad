/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/posix/posix.h"

class TestPosix : public CxxTest::TestSuite 
{
public:
	template<typename T>
	void do_fpclassify()
	{
		T zero = 0.f;
		T one = 1.f;
		T inf = std::numeric_limits<T>::infinity();
		T ninf = -std::numeric_limits<T>::infinity();
		T qnan = std::numeric_limits<T>::quiet_NaN();
		T snan = std::numeric_limits<T>::signaling_NaN();
		T min = std::numeric_limits<T>::min();
		T sub = std::numeric_limits<T>::denorm_min();
		T sub2 = std::numeric_limits<T>::min() / 2;

		TS_ASSERT_EQUALS((int)fpclassify(zero), (int)FP_ZERO);
		TS_ASSERT_EQUALS((int)fpclassify(one), (int)FP_NORMAL);
		TS_ASSERT_EQUALS((int)fpclassify(inf), (int)FP_INFINITE);
		TS_ASSERT_EQUALS((int)fpclassify(ninf), (int)FP_INFINITE);
		TS_ASSERT_EQUALS((int)fpclassify(qnan), (int)FP_NAN);
		TS_ASSERT_EQUALS((int)fpclassify(snan), (int)FP_NAN);
		TS_ASSERT_EQUALS((int)fpclassify(min), (int)FP_NORMAL);
		TS_ASSERT_EQUALS((int)fpclassify(sub), (int)FP_SUBNORMAL);
		TS_ASSERT_EQUALS((int)fpclassify(sub2), (int)FP_SUBNORMAL);

		TS_ASSERT(!isnan(zero));
		TS_ASSERT(!isnan(one));
		TS_ASSERT(!isnan(inf));
		TS_ASSERT(!isnan(ninf));
		TS_ASSERT(isnan(qnan));
		TS_ASSERT(isnan(snan));
		TS_ASSERT(!isnan(min));
		TS_ASSERT(!isnan(sub));
		TS_ASSERT(!isnan(sub2));

		TS_ASSERT(isfinite(zero));
		TS_ASSERT(isfinite(one));
		TS_ASSERT(!isfinite(inf));
		TS_ASSERT(!isfinite(ninf));
		TS_ASSERT(!isfinite(qnan));
		TS_ASSERT(!isfinite(snan));
		TS_ASSERT(isfinite(min));
		TS_ASSERT(isfinite(sub));
		TS_ASSERT(isfinite(sub2));

		TS_ASSERT(!isinf(zero));
		TS_ASSERT(!isinf(one));
		TS_ASSERT(isinf(inf));
		TS_ASSERT(isinf(ninf));
		TS_ASSERT(!isinf(qnan));
		TS_ASSERT(!isinf(snan));
		TS_ASSERT(!isinf(min));
		TS_ASSERT(!isinf(sub));
		TS_ASSERT(!isinf(sub2));

		TS_ASSERT(!isnormal(zero));
		TS_ASSERT(isnormal(one));
		TS_ASSERT(!isnormal(inf));
		TS_ASSERT(!isnormal(ninf));
		TS_ASSERT(!isnormal(qnan));
		TS_ASSERT(!isnormal(snan));
		TS_ASSERT(isnormal(min));
		TS_ASSERT(!isnormal(sub));
		TS_ASSERT(!isnormal(sub2));
	}

	void test_fpclassifyf()
	{
		do_fpclassify<float>();
	}

	void test_fpclassifyd()
	{
		do_fpclassify<double>();
	}
};
