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

#include "maths/FixedVector3D.h"

#define TS_ASSERT_VEC_EQUALS(v, x, y, z) \
	TS_ASSERT_EQUALS(v.X.ToDouble(), x); \
	TS_ASSERT_EQUALS(v.Y.ToDouble(), y); \
	TS_ASSERT_EQUALS(v.Z.ToDouble(), z);

#define TS_ASSERT_VEC_DELTA(v, x, y, z, delta) \
	TS_ASSERT_DELTA(v.X.ToDouble(), x, delta); \
	TS_ASSERT_DELTA(v.Y.ToDouble(), y, delta); \
	TS_ASSERT_DELTA(v.Z.ToDouble(), z, delta);

class TestFixedVector3D : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		CFixedVector3D v1 (fixed::FromInt(1), fixed::FromInt(2), fixed::FromInt(3));
		CFixedVector3D v2 (fixed::FromInt(10), fixed::FromInt(20), fixed::FromInt(30));
		CFixedVector3D v3;
		TS_ASSERT_VEC_EQUALS(v1, 1, 2, 3);
		TS_ASSERT_VEC_EQUALS(v2, 10, 20, 30);
		TS_ASSERT_VEC_EQUALS(v3, 0, 0, 0);
		v3 = v1 + v2;
		TS_ASSERT_VEC_EQUALS(v3, 11, 22, 33);
		v3 += v2;
		TS_ASSERT_VEC_EQUALS(v3, 21, 42, 63);
		v3 -= v2;
		TS_ASSERT_VEC_EQUALS(v3, 11, 22, 33);
		v3 = v1 - v2;
		TS_ASSERT_VEC_EQUALS(v3, -9, -18, -27);
		v3 = -v3;
		TS_ASSERT_VEC_EQUALS(v3, 9, 18, 27);
	}

	void test_Length()
	{
		CFixedVector3D v1 (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
		TS_ASSERT_EQUALS(v1.Length().ToDouble(), 13.0);

		fixed max;
		max.SetInternalValue((i32)0x7fffffff);
		CFixedVector3D v2 (max, fixed::FromInt(0), fixed::FromInt(0));
		TS_ASSERT_EQUALS(v2.Length().ToDouble(), max.ToDouble());

		fixed large;
		large.SetInternalValue((i32)((double)0x7fffffff/sqrt(3.0))+1); // largest value that shouldn't cause overflow
		CFixedVector3D v3 (large, large, large);
		TS_ASSERT_DELTA(v3.Length().ToDouble(), sqrt(3.0)*large.ToDouble(), 0.01);
	}

	void test_Normalize()
	{
		CFixedVector3D v0 (fixed::FromInt(0), fixed::FromInt(0), fixed::FromInt(0));
		v0.Normalize();
		TS_ASSERT_VEC_EQUALS(v0, 0.0, 0.0, 0.0);

		CFixedVector3D v1 (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
		v1.Normalize();
		TS_ASSERT_VEC_DELTA(v1, 3.0/13.0, 4.0/13.0, 12.0/13.0, 0.01);

		fixed max;
		max.SetInternalValue((i32)0x7fffffff);
		CFixedVector3D v2 (max, fixed::FromInt(0), fixed::FromInt(0));
		v2.Normalize();
		TS_ASSERT_VEC_EQUALS(v2, 1.0, 0.0, 0.0);

		fixed large;
		large.SetInternalValue((i32)((double)0x7fffffff/sqrt(3.0))+1); // largest value that shouldn't cause overflow
		CFixedVector3D v3 (large, large, large);
		v3.Normalize();
		TS_ASSERT_VEC_DELTA(v3, 1.0/sqrt(3.0), 1.0/sqrt(3.0), 1.0/sqrt(3.0), 0.01);
	}

	void test_NormalizeTo()
	{
		{
			CFixedVector3D v (fixed::FromInt(0), fixed::FromInt(0), fixed::FromInt(0));
			v.Normalize(fixed::FromInt(1));
			TS_ASSERT_VEC_EQUALS(v, 0.0, 0.0, 0.0);
		}

		{
			CFixedVector3D v (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
			v.Normalize(fixed::FromInt(0));
			TS_ASSERT_VEC_EQUALS(v, 0.0, 0.0, 0.0);
		}

		{
			CFixedVector3D v (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
			v.Normalize(fixed::FromInt(1));
			TS_ASSERT_VEC_DELTA(v, 3.0/13.0, 4.0/13.0, 12.0/13.0, 0.01);
		}

		{
			CFixedVector3D v (fixed::FromInt(3000), fixed::FromInt(4000), fixed::FromInt(12000));
			v.Normalize(fixed::FromInt(1));
			TS_ASSERT_VEC_DELTA(v, 3.0/13.0, 4.0/13.0, 12.0/13.0, 0.01);
		}

		{
			CFixedVector3D v (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
			v.Normalize(fixed::FromInt(100));
			TS_ASSERT_VEC_DELTA(v, 300.0/13.0, 400.0/13.0, 1200.0/13.0, 0.01);
		}

		{
			CFixedVector3D v (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
			v.Normalize(fixed::FromInt(1)/100);
			TS_ASSERT_VEC_DELTA(v, 3.0/1300.0, 4.0/1300.0, 12.0/1300.0, 0.0001);
		}

		{
			CFixedVector3D v (fixed::FromInt(3), fixed::FromInt(4), fixed::FromInt(12));
			v.Normalize(fixed::FromInt(1)/10000);
			TS_ASSERT_VEC_DELTA(v, 3.0/130000.0, 4.0/130000.0, 12.0/130000.0, 0.0001);
		}
	}

	void test_Cross()
	{
		CFixedVector3D v1 (fixed::FromInt(5), fixed::FromInt(6), fixed::FromInt(7));
		CFixedVector3D v2 (fixed::FromInt(8), fixed::FromInt(9), fixed::FromInt(-10));
		CFixedVector3D v3 = v1.Cross(v2);
		TS_ASSERT_VEC_EQUALS(v3, 6*-10 - 7*9, 7*8 - 5*-10, 5*9 - 8*6);
	}

	void test_Dot()
	{
		CFixedVector3D v1 (fixed::FromInt(5), fixed::FromInt(6), fixed::FromInt(7));
		CFixedVector3D v2 (fixed::FromInt(8), fixed::FromInt(9), fixed::FromInt(-10));
		TS_ASSERT_EQUALS(v1.Dot(v2).ToDouble(), 5*8 + 6*9 + 7*-10);
	}
};
