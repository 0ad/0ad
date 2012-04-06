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

#include "maths/BoundingBoxAligned.h"
#include "maths/BoundingBoxOriented.h"

class TestBound : public CxxTest::TestSuite 
{
public:
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
};
