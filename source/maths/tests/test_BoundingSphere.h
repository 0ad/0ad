/* Copyright (C) 2019 Wildfire Games.
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

#include "maths/BoundingBoxAligned.h"
#include "maths/BoundingSphere.h"

#include <cmath>

class TestBoundingSphere : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		CBoundingBoxAligned indentityAABB(CVector3D(-1.0f, -1.0f, -1.0f), CVector3D(1.0f, 1.0f, 1.0f));
		CBoundingSphere bs1 = CBoundingSphere::FromSweptBox(indentityAABB);
		// The radius should be equal to the length of diagonal in an identity cube.
		TS_ASSERT_DELTA(bs1.GetRadius(), sqrtf(3.0f), 1e-5);
		TS_ASSERT_EQUALS(bs1.GetCenter(), CVector3D(0.0f, 0.0f, 0.0f));

		CBoundingBoxAligned translatedAABB;
		CVector3D translate(1.0f, 2.0f, 3.0f);
		indentityAABB.Translate(translate, translatedAABB);
		CBoundingSphere bs2 = CBoundingSphere::FromSweptBox(translatedAABB);
		CVector3D farVertex = translate + CVector3D(1.0f, 1.0f, 1.0f);
		TS_ASSERT_DELTA(bs2.GetRadius(), farVertex.Length(), 1e-5);
		TS_ASSERT_EQUALS(bs2.GetCenter(), CVector3D(0.0f, 0.0f, 0.0f));
	}

	void check_intersections(const CVector3D& pivot)
	{
		// Axis aligned rays for different axis.
		CBoundingSphere bs1(pivot, 1.0f);
		for (size_t i = 0; i < 3; ++i)
		{
			CVector3D origin = pivot, direction;
			origin[i] += 2.0f;
			direction[i] = 1.0f;
			TS_ASSERT_EQUALS(bs1.RayIntersect(origin, -direction), true);
			TS_ASSERT_EQUALS(bs1.RayIntersect(origin, direction), false);
		}

		// Rays inside bounding spheres.
		CBoundingSphere bs2(pivot, 10.0f);
		TS_ASSERT_EQUALS(bs2.RayIntersect(pivot + CVector3D(0.0f, 1.0f, 0.0f), CVector3D(0.0f, 1.0f, 0.0f)), true);
		TS_ASSERT_EQUALS(bs2.RayIntersect(pivot - CVector3D(0.0f, 1.0f, 0.0f), CVector3D(0.0f, 1.0f, 0.0f)), true);

		CBoundingSphere bs3(pivot, 2.0f);
		TS_ASSERT_EQUALS(bs3.RayIntersect(pivot + CVector3D(-4.0f, -2.0f, -4.0f), CVector3D(1.0f, 1.0f, 1.0f).Normalized()), true);
		TS_ASSERT_EQUALS(bs3.RayIntersect(pivot + CVector3D(-4.0f, -1.0f, -4.0f), CVector3D(1.0f, 1.0f, 1.0f).Normalized()), false);
	}

	void test_intersections()
	{
		// Initial positions of bounding spheres.
		CVector3D pivots[] = {
			CVector3D(0.0f, 0.0f, 0.0f),
			CVector3D(1.0f, 2.0f, 3.0f),
			CVector3D(-10.0f, 0.5f, 3.0f)
		};

		for (const CVector3D& pivot : pivots)
			check_intersections(pivot);
	}
};
