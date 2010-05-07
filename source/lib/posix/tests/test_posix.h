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
#ifndef OS_WIN // http://trac.wildfiregames.com/ticket/478
		TS_ASSERT_EQUALS((int)fpclassify(sub), (int)FP_SUBNORMAL);
		TS_ASSERT_EQUALS((int)fpclassify(sub2), (int)FP_SUBNORMAL);
#endif

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
#ifndef OS_WIN // http://trac.wildfiregames.com/ticket/478
		TS_ASSERT(!isnormal(sub));
		TS_ASSERT(!isnormal(sub2));
#endif
	}

	void test_fpclassifyf()
	{
		do_fpclassify<float>();
	}

	void test_fpclassifyd()
	{
		do_fpclassify<double>();
	}

	void test_wcsdup()
	{
		const wchar_t* a = L"test";
		wchar_t* t = wcsdup(a);
		TS_ASSERT_WSTR_EQUALS(t, a);
		free(t);
	}
};
