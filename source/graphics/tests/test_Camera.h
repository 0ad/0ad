/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "graphics/Camera.h"
#include "maths/MathUtil.h"
#include "maths/Vector2D.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

#include <cmath>
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
		camera.SetPerspectiveProjection(1.0f, 101.0f, DEGTORAD(90.0f));
		TS_ASSERT_EQUALS(camera.GetProjectionType(), CCamera::ProjectionType::PERSPECTIVE);
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

	void test_frustum_ortho()
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
		CMatrix3D projection;
		projection.SetOrtho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
		camera.SetProjection(projection);
		TS_ASSERT_EQUALS(camera.GetProjectionType(), CCamera::ProjectionType::CUSTOM);
		camera.UpdateFrustum();

		const std::vector<CPlane> expectedPlanes = {
			CVector4D(1.0f, 0.0f, 0.0f, 10.0f),
			CVector4D(-1.0f, 0.0f, 0.0f, 10.0f),
			CVector4D(0.0f, 1.0f, 0.0f, 10.0f),
			CVector4D(0.0f, -1.0f, 0.0f, 10.0f),
			CVector4D(0.0f, 0.0f, 1.0f, 10.0f),
			CVector4D(0.0f, 0.0f, -1.0f, 10.0f)
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
			if (!found)
				TS_FAIL(frustum[i]);
		}
	}

	bool EqualPlanes(const CPlane& p1, const CPlane& p2) const
	{
		const float EPS = 1e-3f;
		if (std::fabs(p1.m_Dist - p2.m_Dist) >= EPS)
			return false;
		return
			std::fabs(p1.m_Norm.X - p2.m_Norm.X) < EPS &&
			std::fabs(p1.m_Norm.Y - p2.m_Norm.Y) < EPS &&
			std::fabs(p1.m_Norm.Z - p2.m_Norm.Z) < EPS;
	}

	void CompareVectors(const CVector3D& vector1, const CVector3D& vector2, const float EPS)
	{
		TS_ASSERT_DELTA(vector1.X, vector2.X, EPS);
		TS_ASSERT_DELTA(vector1.Y, vector2.Y, EPS);
		TS_ASSERT_DELTA(vector1.Z, vector2.Z, EPS);
	}

	void CompareQuads(const CCamera::Quad& quad, const CCamera::Quad& expectedQuad)
	{
		const float EPS = 1e-4f;
		for (size_t index = 0; index < expectedQuad.size(); ++index)
			CompareVectors(quad[index], expectedQuad[index], EPS);
	}

	void CompareQuadsInWorldSpace(const CCamera& camera, const CCamera::Quad& quad, const CCamera::Quad& expectedQuad)
	{
		const float EPS = 1e-4f;
		for (size_t index = 0; index < expectedQuad.size(); ++index)
		{
			// Transform quad points from camera space to world space.
			CVector3D point = camera.GetOrientation().Transform(quad[index]);

			CompareVectors(point, expectedQuad[index], EPS);
		}
	}

	void test_perspective_plane_points()
	{
		SViewPort viewPort;
		viewPort.m_X = 0;
		viewPort.m_Y = 0;
		viewPort.m_Width = 512;
		viewPort.m_Height = 512;

		CCamera camera;
		camera.SetViewPort(viewPort);
		camera.LookAt(
			CVector3D(10.0f, 20.0f, 10.0f),
			CVector3D(10.0f, 10.0f, 20.0f),
			CVector3D(0.0f, 1.0f, 1.0f).Normalized()
		);
		camera.SetPerspectiveProjection(1.0f, 101.0f, DEGTORAD(90.0f));

		CCamera::Quad quad;

		// Zero distance point is the origin of all camera rays,
		// so all plane points should stay there.
		camera.GetViewQuad(0.0f, quad);
		for (const CVector3D& point : quad)
			TS_ASSERT_EQUALS(point, CVector3D(0.0f, 0.0f, 0.0f));

		// Points lying on the near plane.
		CCamera::Quad expectedNearQuad = {
			CVector3D(-1.0f, -1.0f, 1.0f),
			CVector3D(1.0f, -1.0f, 1.0f),
			CVector3D(1.0f, 1.0f, 1.0f),
			CVector3D(-1.0f, 1.0f, 1.0f)
		};
		CCamera::Quad nearQuad;
		camera.GetViewQuad(camera.GetNearPlane(), nearQuad);
		CompareQuads(nearQuad, expectedNearQuad);

		CCamera::Quad expectedWorldSpaceNearQuad = {
			CVector3D(9.0f, 18.5857868f, 10.0f),
			CVector3D(11.0f, 18.5857868f, 10.0f),
			CVector3D(11.0f, 20.0f, 11.4142132f),
			CVector3D(9.0f, 20.0f, 11.4142132f)
		};
		CompareQuadsInWorldSpace(camera, nearQuad, expectedWorldSpaceNearQuad);

		// Points lying on the far plane.
		CCamera::Quad expectedFarQuad = {
			CVector3D(-101.0f, -101.0f, 101.0f),
			CVector3D(101.0f, -101.0f, 101.0f),
			CVector3D(101.0f, 101.0f, 101.0f),
			CVector3D(-101.0f, 101.0f, 101.0f)
		};
		CCamera::Quad farQuad;
		camera.GetViewQuad(camera.GetFarPlane(), farQuad);
		CompareQuads(farQuad, expectedFarQuad);

		CCamera::Quad expectedWorldSpaceFarQuad = {
			CVector3D(-91.0000153f, -122.8355865f, 10.0f),
			CVector3D(111.0000153f, -122.8355865f, 10.0f),
			CVector3D(111.0000153f, 20.0f, 152.8355865f),
			CVector3D(-91.0000153f, 20.0f, 152.8355865f)
		};
		CompareQuadsInWorldSpace(camera, farQuad, expectedWorldSpaceFarQuad);
	}

	void test_ortho_plane_points()
	{
		SViewPort viewPort;
		viewPort.m_X = 0;
		viewPort.m_Y = 0;
		viewPort.m_Width = 512;
		viewPort.m_Height = 512;

		CCamera camera;
		camera.SetViewPort(viewPort);
		camera.LookAt(
			CVector3D(10.0f, 20.0f, 10.0f),
			CVector3D(10.0f, 10.0f, 20.0f),
			CVector3D(0.0f, 1.0f, 1.0f).Normalized()
		);
		camera.SetOrthoProjection(2.0f, 128.0f, 10.0f);

		// Zero distance is the origin plane of all camera rays,
		// so all plane points should stay there.
		CCamera::Quad quad;
		camera.GetViewQuad(0.0f, quad);
		for (const CVector3D& point : quad)
		{
			constexpr float EPS = 1e-4f;
			TS_ASSERT_DELTA(point.Z, 0.0f, EPS);
		}

		// Points lying on the near plane.
		CCamera::Quad expectedNearQuad = {
			CVector3D(-5.0f, -5.0f, 2.0f),
			CVector3D(5.0f, -5.0f, 2.0f),
			CVector3D(5.0f, 5.0f, 2.0f),
			CVector3D(-5.0f, 5.0f, 2.0f)
		};
		CCamera::Quad nearQuad;
		camera.GetViewQuad(camera.GetNearPlane(), nearQuad);
		CompareQuads(nearQuad, expectedNearQuad);

		CCamera::Quad expectedWorldSpaceNearQuad = {
			CVector3D(4.9999995f, 15.0502520f, 7.8786793f),
			CVector3D(15.0f, 15.0502520f, 7.8786793f),
			CVector3D(15.0f, 22.1213207f, 14.9497480f),
			CVector3D(4.9999995f, 22.1213207f, 14.9497480f)
		};
		CompareQuadsInWorldSpace(camera, nearQuad, expectedWorldSpaceNearQuad);

		// Points lying on the far plane.
		CCamera::Quad expectedFarQuad = {
			CVector3D(-5.0f, -5.0f, 128.0f),
			CVector3D(5.0f, -5.0f, 128.0f),
			CVector3D(5.0f, 5.0f, 128.0f),
			CVector3D(-5.0f, 5.0f, 128.0f)
		};
		CCamera::Quad farQuad;
		camera.GetViewQuad(camera.GetFarPlane(), farQuad);
		CompareQuads(farQuad, expectedFarQuad);

		CCamera::Quad expectedWorldSpaceFarQuad = {
			CVector3D(4.9999995f, -74.0452118f, 96.9741364f),
			CVector3D(15.0f, -74.0452118f, 96.9741364f),
			CVector3D(15.0f, -66.9741364f, 104.0452118f),
			CVector3D(4.9999995f, -66.9741364f, 104.0452118f)
		};
		CompareQuadsInWorldSpace(camera, farQuad, expectedWorldSpaceFarQuad);
	}

	void test_custom_plane_points()
	{
		SViewPort viewPort;
		viewPort.m_X = 0;
		viewPort.m_Y = 0;
		viewPort.m_Width = 512;
		viewPort.m_Height = 512;

		CCamera camera;
		camera.SetViewPort(viewPort);
		camera.LookAt(
			CVector3D(10.0f, 20.0f, 10.0f),
			CVector3D(10.0f, 10.0f, 20.0f),
			CVector3D(0.0f, 1.0f, 1.0f).Normalized()
		);

		CCamera cameraPerspective = camera;
		cameraPerspective.SetPerspectiveProjection(1.0f, 101.0f, DEGTORAD(90.0f));

		CMatrix3D projection;
		projection.SetPerspective(
			cameraPerspective.GetFOV(), cameraPerspective.GetAspectRatio(),
			cameraPerspective.GetNearPlane(), cameraPerspective.GetFarPlane());
		camera.SetProjection(projection);

		const std::vector<float> distances = {
			cameraPerspective.GetNearPlane(),
			(cameraPerspective.GetNearPlane() + cameraPerspective.GetFarPlane()) / 2.0f,
			cameraPerspective.GetFarPlane()
		};

		CCamera::Quad quad, expectedQuad;
		for (const float distance : distances)
		{
			camera.GetViewQuad(distance, quad);
			cameraPerspective.GetViewQuad(distance, expectedQuad);
			CompareQuads(quad, expectedQuad);
		}
	}

	void test_perspective_screen_rays()
	{
		const float EPS = 1e-4f;
		const std::vector<SViewPort> viewPorts = {
			SViewPort{0, 0, 512, 512},
			SViewPort{0, 0, 1024, 768},
			SViewPort{0, 0, 1440, 2536},
		};
		for (const SViewPort& viewPort : viewPorts)
		{
			const CVector3D cameraPosition(10.0f, 20.0f, 10.0f);
			const CVector3D cameraDirection(CVector3D(0.0f, -1.0f, 1.0f).Normalized());
			CCamera camera;
			camera.SetViewPort(viewPort);
			camera.LookAt(
				cameraPosition,
				cameraPosition + cameraDirection * 10.0f,
				CVector3D(0.0f, 1.0f, 1.0f).Normalized()
			);
			camera.SetPerspectiveProjection(1.0f, 101.0f, DEGTORAD(90.0f));

			CVector3D origin, dir;
			camera.BuildCameraRay(viewPort.m_Width / 2, viewPort.m_Height / 2, origin, dir);
			const CVector3D expectedOrigin = cameraPosition;
			const CVector3D expectedDir = cameraDirection;
			CompareVectors(origin, expectedOrigin, EPS);
			CompareVectors(dir, expectedDir, EPS);
		}
	}

	void test_ortho_screen_rays()
	{
		const float EPS = 1e-4f;
		const std::vector<SViewPort> viewPorts = {
			SViewPort{0, 0, 512, 512},
			SViewPort{0, 0, 1024, 768},
			SViewPort{0, 0, 1440, 2536},
		};
		for (const SViewPort& viewPort : viewPorts)
		{
			const CVector3D cameraPosition(10.0f, 20.0f, 10.0f);
			const CVector3D cameraDirection(CVector3D(0.0f, -1.0f, 1.0f).Normalized());
			CCamera camera;
			camera.SetViewPort(viewPort);
			camera.LookAt(
				cameraPosition,
				cameraPosition + cameraDirection * 10.0f,
				CVector3D(0.0f, 1.0f, 1.0f).Normalized()
			);
			camera.SetOrthoProjection(2.0f, 128.0f, 10.0f);

			CVector3D origin, dir;
			camera.BuildCameraRay(viewPort.m_Width / 2, viewPort.m_Height / 2, origin, dir);
			const CVector3D expectedOrigin = cameraPosition;
			const CVector3D expectedDir = cameraDirection;
			CompareVectors(origin, expectedOrigin, EPS);
			CompareVectors(dir, expectedDir, EPS);
		}
	}

	void CompareBoundingBoxes(const CBoundingBoxAligned& bb1, const CBoundingBoxAligned& bb2)
	{
		constexpr float EPS = 1e-3f;
		CompareVectors(bb1[0], bb2[0], EPS);
		CompareVectors(bb1[1], bb2[1], EPS);
	}

	void test_viewport_bounds_perspective()
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
		camera.SetPerspectiveProjection(1.0f, 101.0f, DEGTORAD(90.0f));
		camera.UpdateFrustum();

		struct TestCase
		{
			CBoundingBoxAligned worldSpaceBoundingBox;
			CBoundingBoxAligned expectedViewPortBoundingBox;
		};
		const TestCase testCases[] = {
			// Box is in front of the camera.
			{
				{{-1.0f, 0.0f, 5.0f}, {1.0f, 0.0f, 7.0f}},
				{{-0.2f, 0.0f, 0.616f}, {0.2f, 0.0f, 0.731429f}}
			},
			// Box is out of the camera view.
			{
				{{-10.0f, -1.0f, 5.0f}, {-8.0f, 1.0f, 7.0f}},
				{}
			},
			{
				{{-1.0f, -10.0f, 5.0f}, {1.0f, -8.0f, 7.0f}},
				{}
			},
			// Box is in the bottom part of the camera view.
			{
				{{-1.0f, -3.0f, 5.0f}, {1.0f, -3.0f, 7.0f}},
				{{-0.2f, -0.6f, 0.616f}, {0.2f, -0.428571f, 0.731429f}}
			},
			{
				{{-1.0f, -3.0f, 0.0f}, {1.0f, -3.0f, 7.0f}},
				{{-1.0f, -3.0f, -1.0f}, {1.0f, -0.428571f, 0.731429f}}
			},
			{
				{{-1.0f, -3.0f, -7.0f}, {1.0f, -3.0f, 7.0f}},
				{{-1.0f, -3.0f, -1.0f}, {1.0f, -0.428571f, 0.731429f}}
			},
		};

		for (const TestCase& testCase : testCases)
		{
			TS_ASSERT(testCase.worldSpaceBoundingBox[0].X <= testCase.worldSpaceBoundingBox[1].X);
			TS_ASSERT(testCase.worldSpaceBoundingBox[0].Y <= testCase.worldSpaceBoundingBox[1].Y);
			TS_ASSERT(testCase.worldSpaceBoundingBox[0].Z <= testCase.worldSpaceBoundingBox[1].Z);

			const CBoundingBoxAligned result =
				camera.GetBoundsInViewPort(testCase.worldSpaceBoundingBox);
			if (testCase.expectedViewPortBoundingBox.IsEmpty())
			{
				TS_ASSERT(result.IsEmpty());
			}
			else
				CompareBoundingBoxes(result, testCase.expectedViewPortBoundingBox);
		}
	}

	void test_viewport_bounds_ortho()
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
		camera.SetOrthoProjection(1.0f, 101.0f, 2.0f);
		camera.UpdateFrustum();

		struct TestCase
		{
			CBoundingBoxAligned worldSpaceBoundingBox;
			CBoundingBoxAligned expectedViewPortBoundingBox;
		};
		const TestCase testCases[] = {
			// A box is in front of the camera.
			{
				{{-1.0f, 0.0f, 5.0f}, {1.0f, 0.0f, 7.0f}},
				{{-1.0f, 0.0f, -1.1599f}, {1.0f, 0.0f,-1.1200f}}
			},
			// A box is out of the camera view.
			{
				{{-10.0f, -1.0f, 5.0f}, {-8.0f, 1.0f, 7.0f}},
				{}
			},
			{
				{{-1.0f, -10.0f, 5.0f}, {1.0f, -8.0f, 7.0f}},
				{}
			},
			// The camera is inside a box.
			{
				{{-1.0f, 0.0f, -7.0f}, {1.0f, 0.0f, 7.0f}},
				{{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}}
			},
			// A box intersects with the near plane.
			{
				{{-1.0f, 0.0f, 0.5f}, {1.0f, 0.0f, 7.0f}},
				{{-1.0f, 0.0f, -1.1599f}, {1.0f, 0.0f, -1.1599f}}
			},
		};

		for (const TestCase& testCase : testCases)
		{
			TS_ASSERT(testCase.worldSpaceBoundingBox[0].X <= testCase.worldSpaceBoundingBox[1].X);
			TS_ASSERT(testCase.worldSpaceBoundingBox[0].Y <= testCase.worldSpaceBoundingBox[1].Y);
			TS_ASSERT(testCase.worldSpaceBoundingBox[0].Z <= testCase.worldSpaceBoundingBox[1].Z);

			const CBoundingBoxAligned result =
				camera.GetBoundsInViewPort(testCase.worldSpaceBoundingBox);
			if (testCase.expectedViewPortBoundingBox.IsEmpty())
			{
				TS_ASSERT(result.IsEmpty());
			}
			else
				CompareBoundingBoxes(result, testCase.expectedViewPortBoundingBox);
		}
	}

};
