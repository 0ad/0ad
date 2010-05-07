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

#include <cstdlib>
#include <cmath>
#include "maths/Matrix3D.h"
#include "maths/Quaternion.h"

class TestMatrix : public CxxTest::TestSuite 
{
public:
	void test_inverse()
	{
		CMatrix3D m;
		srand(0);
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				m._data[j] = -1.0f + 2.0f*(rand()/(float)RAND_MAX);
			}
			CMatrix3D n;
			m.GetInverse(n);
			m *= n;
			// verify identity has 1s on diagonal and 0 otherwise
			for (int x = 0; x < 4; ++x)
			{
				for (int y = 0; y < 4; ++y)
				{
					const float expected = (x==y)? 1.0f : 0.0f;
					TS_ASSERT_DELTA(m(x,y), expected, 0.0002f);
				}
			}
		}
	}

	void test_quats()
	{
		srand(0);
		for (int i = 0; i < 4; ++i)
		{
			CQuaternion q;
			q.FromEulerAngles(
				-6.28f + 12.56f*(rand()/(float)RAND_MAX),
				-6.28f + 12.56f*(rand()/(float)RAND_MAX),
				-6.28f + 12.56f*(rand()/(float)RAND_MAX)
				);
			CMatrix3D m;
			q.ToMatrix(m);
			CQuaternion q2 = m.GetRotation();

			// Quaternions (x,y,z,w) and (-x,-y,-z,-w) are equivalent when
			// interpreted as rotations, so it doesn't matter which we get
			const bool ok_oneway = 
				feq(q2.m_W, q.m_W) &&
				feq(q2.m_V.X, q.m_V.X) &&
				feq(q2.m_V.Y, q.m_V.Y) &&
				feq(q2.m_V.Z, q.m_V.Z);
			const bool ok_otherway =
				feq(q2.m_W, -q.m_W) &&
				feq(q2.m_V.X, -q.m_V.X) &&
				feq(q2.m_V.Y, -q.m_V.Y) &&
				feq(q2.m_V.Z, -q.m_V.Z);
			TS_ASSERT(ok_oneway ^ ok_otherway);
		}
	}
};


