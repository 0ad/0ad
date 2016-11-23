/* Copyright (C) 2012 Wildfire Games.
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
#include "maths/BoundingBoxAligned.h"
#include "maths/BoundingBoxOriented.h"
#include "maths/Matrix3D.h"

#define TS_ASSERT_VEC_DELTA(v, x, y, z, delta) \
	TS_ASSERT_DELTA(v.X, x, delta); \
	TS_ASSERT_DELTA(v.Y, y, delta); \
	TS_ASSERT_DELTA(v.Z, z, delta);

class TestBound : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CxxTest::setAbortTestOnFail(true);
	}

	void test_empty_aabb()
	{
		CBoundingBoxAligned bound;
		TS_ASSERT(bound.IsEmpty());
		bound += CVector3D(1, 2, 3);
		TS_ASSERT(! bound.IsEmpty());
		bound.SetEmpty();
		TS_ASSERT(bound.IsEmpty());
	}

	void test_empty_obb()
	{
		CBoundingBoxOriented bound;
		TS_ASSERT(bound.IsEmpty());
		bound.m_Basis[0] = CVector3D(1,0,0);
		bound.m_Basis[1] = CVector3D(0,1,0);
		bound.m_Basis[2] = CVector3D(0,0,1);
		bound.m_HalfSizes = CVector3D(1,2,3);
		TS_ASSERT(!bound.IsEmpty());
		bound.SetEmpty();
		TS_ASSERT(bound.IsEmpty());
	}

	void test_extend_vector()
	{
		CBoundingBoxAligned bound;
		CVector3D v (1, 2, 3);
		bound += v;

		CVector3D centre;
		bound.GetCentre(centre);
		TS_ASSERT_EQUALS(centre, v);
	}

	void test_extend_bound()
	{
		CBoundingBoxAligned bound;
		CVector3D v (1, 2, 3);
		CBoundingBoxAligned b (v, v);
		bound += b;

		CVector3D centre;
		bound.GetCentre(centre);
		TS_ASSERT_EQUALS(centre, v);
	}

	void test_aabb_to_obb_translation()
	{
		CBoundingBoxAligned aabb(CVector3D(-1,-2,-1), CVector3D(1,2,1));

		CMatrix3D translation;
		translation.SetTranslation(CVector3D(1,3,7));

		CBoundingBoxOriented result;
		aabb.Transform(translation, result);

		TS_ASSERT_VEC_DELTA(result.m_Center,    1.f, 3.f, 7.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[0],  1.f, 0.f, 0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[1],  0.f, 1.f, 0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[2],  0.f, 0.f, 1.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_HalfSizes, 1.f, 2.f, 1.f, 1e-7f);
	}

	void test_aabb_to_obb_rotation_around_origin()
	{
		// rotate a 4x3x3 AABB centered at (5,0,0) 90 degrees CCW around the Z axis, and verify that the
		// resulting OBB is correct
		CBoundingBoxAligned aabb(CVector3D(3, -1.5f, -1.5f), CVector3D(7, 1.5f, 1.5f));

		CMatrix3D rotation;
		rotation.SetZRotation(float(M_PI)/2.f);

		CBoundingBoxOriented result;
		aabb.Transform(rotation, result);

		TS_ASSERT_VEC_DELTA(result.m_Center,    0.f, 5.f, 0.f, 1e-6f); // involves some trigonometry, lower precision
		TS_ASSERT_VEC_DELTA(result.m_Basis[0],  0.f, 1.f, 0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[1], -1.f, 0.f, 0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[2],  0.f, 0.f, 1.f, 1e-7f);
	}

	void test_aabb_to_obb_rotation_around_point()
	{
		// rotate a 4x3x3 AABB centered at (5,0,0) 45 degrees CW around the Z axis through (2,0,0)
		CBoundingBoxAligned aabb(CVector3D(3, -1.5f, -1.5f), CVector3D(7, 1.5f, 1.5f));

		// move everything so (2,0,0) becomes the origin, do the rotation, then move everything back
		CMatrix3D translate;
		CMatrix3D rotate;
		CMatrix3D translateBack;
		translate.SetTranslation(-2.f, 0, 0);
		rotate.SetZRotation(-float(M_PI)/4.f);
		translateBack.SetTranslation(2.f, 0, 0);

		CMatrix3D transform;
		transform.SetIdentity();
		transform.Concatenate(translate);
		transform.Concatenate(rotate);
		transform.Concatenate(translateBack);

		CBoundingBoxOriented result;
		aabb.Transform(transform, result);

		const float invSqrt2 = 1.f/sqrtf(2.f);

		TS_ASSERT_VEC_DELTA(result.m_Center,   3*invSqrt2 + 2, -3*invSqrt2, 0.f, 1e-6f); // involves some trigonometry, lower precision
		TS_ASSERT_VEC_DELTA(result.m_Basis[0],       invSqrt2,   -invSqrt2, 0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[1],       invSqrt2,    invSqrt2, 0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[2],            0.f,         0.f, 1.f, 1e-7f);
	}

	void test_aabb_to_obb_scale()
	{
		CBoundingBoxAligned aabb(CVector3D(3, -1.5f, -1.5f), CVector3D(7, 1.5f, 1.5f));

		CMatrix3D scale;
		scale.SetScaling(1.f, 3.f, 7.f);

		CBoundingBoxOriented result;
		aabb.Transform(scale, result);

		TS_ASSERT_VEC_DELTA(result.m_Center,    5.f,  0.f,   0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_HalfSizes, 2.f, 4.5f, 10.5f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[0],  1.f,  0.f,   0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[1],  0.f,  1.f,   0.f, 1e-7f);
		TS_ASSERT_VEC_DELTA(result.m_Basis[2],  0.f,  0.f,   1.f, 1e-7f);
	}

	// Verify that ray/OBB intersection is correctly determined in degenerate case where the
	// box has zero size in one of its dimensions.
	void test_degenerate_obb_ray_intersect()
	{
		// create OBB of a flat 1x1 square in the X/Z plane, with 0 size in the Y dimension
		CBoundingBoxOriented bound;
		bound.m_Basis[0] = CVector3D(1,0,0); // X
		bound.m_Basis[1] = CVector3D(0,1,0); // Y
		bound.m_Basis[2] = CVector3D(0,0,1); // Z
		bound.m_HalfSizes[0] = 1.f;
		bound.m_HalfSizes[1] = 0.f; // no height, i.e. a "flat" OBB
		bound.m_HalfSizes[2] = 1.f;
		bound.m_Center = CVector3D(0,0,0);

		// create two rays; one that should hit the OBB, and one that should miss it
		CVector3D ray1origin(-3.5f, 3.f, 0.f);
		CVector3D ray1direction(1.f, -1.f, 0.f);
		CVector3D ray2origin(-4.5f, 3.f, 0.f);
		CVector3D ray2direction(1.f, -1.f, 0.f);

		float tMin, tMax;
		TSM_ASSERT("Ray 1 should intersect the OBB", bound.RayIntersect(ray1origin, ray1direction, tMin, tMax));
		TSM_ASSERT("Ray 2 should not intersect the OBB", !bound.RayIntersect(ray2origin, ray2direction, tMin, tMax));
	}

	// Verify that transforming a flat AABB to an OBB does not produce NaN basis vectors in the
	// resulting OBB (see http://trac.wildfiregames.com/ticket/1121)
	void test_degenerate_aabb_to_obb_transform()
	{
		// create a flat AABB, transform it with some matrix (can even be the identity matrix),
		// and verify that the result does not contain any NaN values in its basis vectors
		// and/or half-sizes
		CBoundingBoxAligned flatAabb(CVector3D(-1,0,-1), CVector3D(1,0,1));

		CMatrix3D transform;
		transform.SetIdentity();

		CBoundingBoxOriented result;
		flatAabb.Transform(transform, result);

		TS_ASSERT(!isnan(result.m_Basis[0].X) && !isnan(result.m_Basis[0].Y) && !isnan(result.m_Basis[0].Z));
		TS_ASSERT(!isnan(result.m_Basis[1].X) && !isnan(result.m_Basis[1].Y) && !isnan(result.m_Basis[1].Z));
		TS_ASSERT(!isnan(result.m_Basis[2].X) && !isnan(result.m_Basis[2].Y) && !isnan(result.m_Basis[2].Z));
	}

};
