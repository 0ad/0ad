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

#include "graphics/Camera.h"
#include "maths/MathUtil.h"
#include "maths/Vector3D.h"

#include <vector>

class TestCamera : public CxxTest::TestSuite
{
public:
	void test_frustum_perspective()
	{
		SViewPort viewPort;
		viewPort.m_X = 0;
		viewPort.m_Y = 0;
		viewPort.m_Width = 512;
		viewPort.m_Height = 512;

		CCamera camera;
		camera.SetViewPort(viewPort);
		camera.LookAlong(
			CVector3D(0.0f, 0.0f, 0.0f),
			CVector3D(0.0f, 0.0f, 1.0f),
			CVector3D(0.0f, 1.0f, 0.0f)
		);
		camera.SetProjection(1.0f, 101.0f, DEGTORAD(90.0f));
		camera.UpdateFrustum();

		const float sqrt2 = sqrtf(2.0f) / 2.0f;
		const std::vector<CPlane> expectedPlanes = {
			CVector4D(sqrt2, 0.0f, sqrt2, 0.0f),
			CVector4D(-sqrt2, 0.0f, sqrt2, 0.0f),
			CVector4D(0.0f, sqrt2, sqrt2, 0.0f),
			CVector4D(0.0f, -sqrt2, sqrt2, 0.0f),
			CVector4D(0.0f, 0.0f, -1.0f, 101.0f),
			CVector4D(0.0f, 0.0f, 1.0f, -1.0f),
		};
		CheckFrustumPlanes(camera.GetFrustum(), expectedPlanes);
	}

	// Order of planes is unknown. So use interactive checker.
	void CheckFrustumPlanes(const CFrustum& frustum, const std::vector<CPlane>& expectedPlanes)
	{
		TS_ASSERT_EQUALS(frustum.GetNumPlanes(), expectedPlanes.size());
		std::set<size_t> indices;
		for (size_t i = 0; i < expectedPlanes.size(); ++i)
			indices.insert(i);

		for (size_t i = 0; i < frustum.GetNumPlanes(); ++i)
		{
			bool found = false;
			for (size_t j : indices)
			{
				if (EqualPlanes(frustum[i], expectedPlanes[j]))
				{
					found = true;
					indices.erase(j);
					break;
				}
			}
			TS_ASSERT(found);
		}
	}

	bool EqualPlanes(const CPlane& p1, const CPlane& p2) const
	{
		const float EPS = 1e-3f;
		if (std::fabsf(p1.m_Dist - p2.m_Dist) >= EPS)
			return false;
		return
			std::fabsf(p1.m_Norm.X - p2.m_Norm.X) < EPS &&
			std::fabsf(p1.m_Norm.Y - p2.m_Norm.Y) < EPS &&
			std::fabsf(p1.m_Norm.Z - p2.m_Norm.Z) < EPS;
	}
};
