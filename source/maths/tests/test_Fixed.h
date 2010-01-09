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

#include "maths/Fixed.h"
#include "maths/MathUtil.h"

class TestFixed : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		CFixed_23_8 a = CFixed_23_8::FromInt(1234567);
		TS_ASSERT_EQUALS((a + a).ToDouble(), 2469134);
		TS_ASSERT_EQUALS((a - a).ToDouble(), 0);
		TS_ASSERT_EQUALS((-a).ToDouble(), -1234567);
		TS_ASSERT_EQUALS((a / a).ToDouble(), 1);
		TS_ASSERT_EQUALS((a / 127).ToDouble(), 9721);
		TS_ASSERT_EQUALS((a * 2).ToDouble(), 2469134);
	}

	void test_FromInt()
	{
		CFixed_23_8 a = CFixed_23_8::FromInt(123);
		TS_ASSERT_EQUALS(a.ToFloat(), 123.0f);
		TS_ASSERT_EQUALS(a.ToDouble(), 123.0);
		TS_ASSERT_EQUALS(a.ToInt_RoundToZero(), 123);
	}

	void test_FromFloat()
	{
		CFixed_23_8 a = CFixed_23_8::FromFloat(123.125f);
		TS_ASSERT_EQUALS(a.ToFloat(), 123.125f);
		TS_ASSERT_EQUALS(a.ToDouble(), 123.125);

		CFixed_23_8 b = CFixed_23_8::FromFloat(-123.125f);
		TS_ASSERT_EQUALS(b.ToFloat(), -123.125f);
		TS_ASSERT_EQUALS(b.ToDouble(), -123.125);

		CFixed_23_8 c = CFixed_23_8::FromFloat(INFINITY);
		TS_ASSERT_EQUALS(c.GetInternalValue(), (i32)0);

		CFixed_23_8 d = CFixed_23_8::FromFloat(-INFINITY);
		TS_ASSERT_EQUALS(d.GetInternalValue(), (i32)0);

		CFixed_23_8 e = CFixed_23_8::FromFloat(NAN);
		TS_ASSERT_EQUALS(e.GetInternalValue(), (i32)0);
	}

	void test_FromDouble()
	{
		CFixed_23_8 a = CFixed_23_8::FromDouble(123.125);
		TS_ASSERT_EQUALS(a.ToFloat(), 123.125f);
		TS_ASSERT_EQUALS(a.ToDouble(), 123.125);

		CFixed_23_8 b = CFixed_23_8::FromDouble(-123.125);
		TS_ASSERT_EQUALS(b.ToFloat(), -123.125f);
		TS_ASSERT_EQUALS(b.ToDouble(), -123.125);

		CFixed_23_8 c = CFixed_23_8::FromDouble(INFINITY);
		TS_ASSERT_EQUALS(c.GetInternalValue(), (i32)0);

		CFixed_23_8 d = CFixed_23_8::FromDouble(-INFINITY);
		TS_ASSERT_EQUALS(d.GetInternalValue(), (i32)0);

		CFixed_23_8 e = CFixed_23_8::FromDouble(NAN);
		TS_ASSERT_EQUALS(e.GetInternalValue(), (i32)0);
	}

	void test_FromFloat_Rounding()
	{
		double eps = pow(2.0, -8.0);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(eps).ToDouble(), eps);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(eps * 0.5625).ToDouble(), eps);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(eps * 0.5).ToDouble(), eps);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(eps * 0.4375).ToDouble(), 0.0);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-eps).ToDouble(), -eps);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-eps * 0.5625).ToDouble(), -eps);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-eps * 0.5).ToDouble(), -eps);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-eps * 0.4375).ToDouble(), 0.0);
	}

	void test_RoundToZero()
	{
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(10.f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(10.1f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(10.5f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(10.99f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(0.1f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(0.0f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-0.1f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-0.99f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-1.0f).ToInt_RoundToZero(), -1);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-2.0f).ToInt_RoundToZero(), -2);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-2.5f).ToInt_RoundToZero(), -2);
		TS_ASSERT_EQUALS(CFixed_23_8::FromFloat(-2.99f).ToInt_RoundToZero(), -2);
	}

	// TODO: test all the arithmetic operators

	void test_Atan2()
	{
		typedef CFixed_23_8 f;

		// Special cases from atan2 man page:
		TS_ASSERT_DELTA(atan2_approx(f::FromInt(0), f::FromInt(-1)).ToDouble(), M_PI, 0.01);
		TS_ASSERT_EQUALS(atan2_approx(f::FromInt(0), f::FromInt(+1)).ToDouble(), 0);
		TS_ASSERT_DELTA(atan2_approx(f::FromInt(-1), f::FromInt(0)).ToDouble(), -M_PI_2, 0.01);
		TS_ASSERT_DELTA(atan2_approx(f::FromInt(+1), f::FromInt(0)).ToDouble(), +M_PI_2, 0.01);
		TS_ASSERT_EQUALS(atan2_approx(f::FromInt(0), f::FromInt(0)).ToDouble(), 0);

		// Test that it approximately matches libc's atan2
		#define T(y, x) TS_ASSERT_DELTA(atan2_approx(f::FromDouble(y), f::FromDouble(x)).ToDouble(), atan2((double)(y), (double)(x)), 0.078)

		// Quadrants
		T(1, 1);
		T(-1, 1);
		T(1, -1);
		T(-1, -1);

		// Scale
		T(10, 10);
		T(100, 100);
		T(1000, 1000);
		T(10000, 10000);

		// Some arbitrary cases
		T(2, 1);
		T(3, 1);
		T(4, 1);
		T(10, 1);
		T(1000, 1);
		T(1, 2);
		T(1, 3);
		T(1, 4);
		T(1, 10);
		T(1, 1000);

		// The highest-error region
		for (double x = 2.0; x < 4.0; x += 0.01)
		{
			T(1, x);
		}

		#undef T
	}
};
