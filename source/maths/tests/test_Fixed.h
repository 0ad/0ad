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

#include "maths/Fixed.h"
#include "maths/MathUtil.h"

class TestFixed : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		fixed a = fixed::FromInt(12345);
		TS_ASSERT_EQUALS((a + a).ToDouble(), 24690);
		TS_ASSERT_EQUALS((a - a).ToDouble(), 0);
		TS_ASSERT_EQUALS((-a).ToDouble(), -12345);
		TS_ASSERT_EQUALS((a / a).ToDouble(), 1);
		TS_ASSERT_EQUALS((a / 823).ToDouble(), 15);
		TS_ASSERT_EQUALS((a * 2).ToDouble(), 24690);
	}

	void test_FromInt()
	{
		fixed a = fixed::FromInt(123);
		TS_ASSERT_EQUALS(a.ToFloat(), 123.0f);
		TS_ASSERT_EQUALS(a.ToDouble(), 123.0);
		TS_ASSERT_EQUALS(a.ToInt_RoundToZero(), 123);
	}

	void test_FromFloat()
	{
		fixed a = fixed::FromFloat(123.125f);
		TS_ASSERT_EQUALS(a.ToFloat(), 123.125f);
		TS_ASSERT_EQUALS(a.ToDouble(), 123.125);

		fixed b = fixed::FromFloat(-123.125f);
		TS_ASSERT_EQUALS(b.ToFloat(), -123.125f);
		TS_ASSERT_EQUALS(b.ToDouble(), -123.125);

		fixed c = fixed::FromFloat(INFINITY);
		TS_ASSERT_EQUALS(c.GetInternalValue(), (i32)0);

		fixed d = fixed::FromFloat(-INFINITY);
		TS_ASSERT_EQUALS(d.GetInternalValue(), (i32)0);

		fixed e = fixed::FromFloat(NAN);
		TS_ASSERT_EQUALS(e.GetInternalValue(), (i32)0);
	}

	void test_FromDouble()
	{
		fixed a = fixed::FromDouble(123.125);
		TS_ASSERT_EQUALS(a.ToFloat(), 123.125f);
		TS_ASSERT_EQUALS(a.ToDouble(), 123.125);

		fixed b = fixed::FromDouble(-123.125);
		TS_ASSERT_EQUALS(b.ToFloat(), -123.125f);
		TS_ASSERT_EQUALS(b.ToDouble(), -123.125);

		fixed c = fixed::FromDouble(INFINITY);
		TS_ASSERT_EQUALS(c.GetInternalValue(), (i32)0);

		fixed d = fixed::FromDouble(-INFINITY);
		TS_ASSERT_EQUALS(d.GetInternalValue(), (i32)0);

		fixed e = fixed::FromDouble(NAN);
		TS_ASSERT_EQUALS(e.GetInternalValue(), (i32)0);
	}

	void test_FromFloat_Rounding()
	{
		double eps = pow(2.0, -16.0);
		TS_ASSERT_EQUALS(fixed::FromFloat(eps).ToDouble(), eps);
		TS_ASSERT_EQUALS(fixed::FromFloat(eps * 0.5625).ToDouble(), eps);
		TS_ASSERT_EQUALS(fixed::FromFloat(eps * 0.5).ToDouble(), eps);
		TS_ASSERT_EQUALS(fixed::FromFloat(eps * 0.4375).ToDouble(), 0.0);
		TS_ASSERT_EQUALS(fixed::FromFloat(-eps).ToDouble(), -eps);
		TS_ASSERT_EQUALS(fixed::FromFloat(-eps * 0.5625).ToDouble(), -eps);
		TS_ASSERT_EQUALS(fixed::FromFloat(-eps * 0.5).ToDouble(), -eps);
		TS_ASSERT_EQUALS(fixed::FromFloat(-eps * 0.4375).ToDouble(), 0.0);
	}

	void test_FromString()
	{
		TS_ASSERT_EQUALS(fixed::FromString("").ToDouble(), 0.0);
		TS_ASSERT_EQUALS(fixed::FromString("123").ToDouble(), 123.0);
		TS_ASSERT_EQUALS(fixed::FromString("+123").ToDouble(), 123.0);
		TS_ASSERT_EQUALS(fixed::FromString("-123").ToDouble(), -123.0);
		TS_ASSERT_EQUALS(fixed::FromString("123.5").ToDouble(), 123.5);
		TS_ASSERT_EQUALS(fixed::FromString("-123.5").ToDouble(), -123.5);
		TS_ASSERT_EQUALS(fixed::FromString(".5").ToDouble(), 0.5);
		TS_ASSERT_EQUALS(fixed::FromString("5.").ToDouble(), 5.0);
		TS_ASSERT_EQUALS(fixed::FromString("254.391").GetInternalValue(), 16671768);

		TS_ASSERT_EQUALS(fixed::FromString("123x456").ToDouble(), 123.0);
		TS_ASSERT_EQUALS(fixed::FromString("1.00002").ToDouble(), 1.0 + 1.0/65536.0);
		TS_ASSERT_EQUALS(fixed::FromString("1.0000200000000000000000").ToDouble(), 1.0 + 1.0/65536.0);
		TS_ASSERT_EQUALS(fixed::FromString("1.000009").ToDouble(), 1.0);
		TS_ASSERT_EQUALS(fixed::FromString("0.9999999999999999999999").ToDouble(), 1.0 - 1.0/65536.0);

		// TODO: could test more large/small numbers, errors, etc
	}

	void test_RoundToZero()
	{
		TS_ASSERT_EQUALS(fixed::FromFloat(10.f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(fixed::FromFloat(10.1f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(fixed::FromFloat(10.5f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(fixed::FromFloat(10.99f).ToInt_RoundToZero(), 10);
		TS_ASSERT_EQUALS(fixed::FromFloat(0.1f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(fixed::FromFloat(0.0f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(fixed::FromFloat(-0.1f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(fixed::FromFloat(-0.99f).ToInt_RoundToZero(), 0);
		TS_ASSERT_EQUALS(fixed::FromFloat(-1.0f).ToInt_RoundToZero(), -1);
		TS_ASSERT_EQUALS(fixed::FromFloat(-2.0f).ToInt_RoundToZero(), -2);
		TS_ASSERT_EQUALS(fixed::FromFloat(-2.5f).ToInt_RoundToZero(), -2);
		TS_ASSERT_EQUALS(fixed::FromFloat(-2.99f).ToInt_RoundToZero(), -2);
	}

	// TODO: test all the arithmetic operators

	void test_Sqrt()
	{
		TS_ASSERT_EQUALS(fixed::FromDouble(1.0).Sqrt().ToDouble(), 1.0);
		TS_ASSERT_DELTA(fixed::FromDouble(2.0).Sqrt().ToDouble(), sqrt(2.0), 1.0/65536.0);
		TS_ASSERT_EQUALS(fixed::FromDouble(32400.0).Sqrt().ToDouble(), 180.0);
		TS_ASSERT_EQUALS(fixed::FromDouble(0.0625).Sqrt().ToDouble(), 0.25);
		TS_ASSERT_EQUALS(fixed::FromDouble(-1.0).Sqrt().ToDouble(), 0.0);
		TS_ASSERT_EQUALS(fixed::FromDouble(0.0).Sqrt().ToDouble(), 0.0);
	}

	void test_Atan2()
	{
		// Special cases from atan2 man page:
		TS_ASSERT_DELTA(atan2_approx(fixed::FromInt(0), fixed::FromInt(-1)).ToDouble(), M_PI, 0.01);
		TS_ASSERT_EQUALS(atan2_approx(fixed::FromInt(0), fixed::FromInt(+1)).ToDouble(), 0);
		TS_ASSERT_DELTA(atan2_approx(fixed::FromInt(-1), fixed::FromInt(0)).ToDouble(), -M_PI_2, 0.01);
		TS_ASSERT_DELTA(atan2_approx(fixed::FromInt(+1), fixed::FromInt(0)).ToDouble(), +M_PI_2, 0.01);
		TS_ASSERT_EQUALS(atan2_approx(fixed::FromInt(0), fixed::FromInt(0)).ToDouble(), 0);

		// Test that it approximately matches libc's atan2
		#define T(y, x) TS_ASSERT_DELTA(atan2_approx(fixed::FromDouble(y), fixed::FromDouble(x)).ToDouble(), atan2((double)(y), (double)(x)), 0.072)

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
