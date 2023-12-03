/* Copyright (C) 2023 Wildfire Games.
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

#include "precompiled.h"

#include "Camera.h"

#include "graphics/HFTracer.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "maths/Vector2D.h"
#include "maths/Vector4D.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/WaterManager.h"

CCamera::CCamera()
{
	// Set viewport to something anything should handle, but should be initialised
	// to window size before use.
	m_ViewPort.m_X = 0;
	m_ViewPort.m_Y = 0;
	m_ViewPort.m_Width = 800;
	m_ViewPort.m_Height = 600;
}

CCamera::~CCamera() = default;

void CCamera::SetProjection(const CMatrix3D& matrix)
{
	m_ProjType = ProjectionType::CUSTOM;
	m_ProjMat = matrix;
}

void CCamera::SetProjectionFromCamera(const CCamera& camera)
{
	m_ProjType = camera.m_ProjType;
	m_NearPlane = camera.m_NearPlane;
	m_FarPlane = camera.m_FarPlane;
	if (m_ProjType == ProjectionType::PERSPECTIVE)
	{
		m_FOV = camera.m_FOV;
	}
	else if (m_ProjType == ProjectionType::ORTHO)
	{
		m_OrthoScale = camera.m_OrthoScale;
	}
	m_ProjMat = camera.m_ProjMat;
}

void CCamera::SetOrthoProjection(float nearp, float farp, float scale)
{
	m_ProjType = ProjectionType::ORTHO;
	m_NearPlane = nearp;
	m_FarPlane = farp;
	m_OrthoScale = scale;

	const float halfHeight = 0.5f * m_OrthoScale;
	const float halfWidth = halfHeight * GetAspectRatio();
	m_ProjMat.SetOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, m_NearPlane, m_FarPlane);
}

void CCamera::SetPerspectiveProjection(float nearp, float farp, float fov)
{
	m_ProjType = ProjectionType::PERSPECTIVE;
	m_NearPlane = nearp;
	m_FarPlane = farp;
	m_FOV = fov;

	m_ProjMat.SetPerspective(m_FOV, GetAspectRatio(), m_NearPlane, m_FarPlane);
}

// Updates the frustum planes. Should be called
// everytime the view or projection matrices are
// altered.
void CCamera::UpdateFrustum(const CBoundingBoxAligned& scissor)
{
	CMatrix3D MatFinal;
	CMatrix3D MatView;

	m_Orientation.GetInverse(MatView);

	MatFinal = m_ProjMat * MatView;

	m_ViewFrustum.SetNumPlanes(6);

	// get the RIGHT plane
	m_ViewFrustum[0].m_Norm.X = scissor[1].X*MatFinal._41 - MatFinal._11;
	m_ViewFrustum[0].m_Norm.Y = scissor[1].X*MatFinal._42 - MatFinal._12;
	m_ViewFrustum[0].m_Norm.Z = scissor[1].X*MatFinal._43 - MatFinal._13;
	m_ViewFrustum[0].m_Dist   = scissor[1].X*MatFinal._44 - MatFinal._14;

	// get the LEFT plane
	m_ViewFrustum[1].m_Norm.X = -scissor[0].X*MatFinal._41 + MatFinal._11;
	m_ViewFrustum[1].m_Norm.Y = -scissor[0].X*MatFinal._42 + MatFinal._12;
	m_ViewFrustum[1].m_Norm.Z = -scissor[0].X*MatFinal._43 + MatFinal._13;
	m_ViewFrustum[1].m_Dist   = -scissor[0].X*MatFinal._44 + MatFinal._14;

	// get the BOTTOM plane
	m_ViewFrustum[2].m_Norm.X = -scissor[0].Y*MatFinal._41 + MatFinal._21;
	m_ViewFrustum[2].m_Norm.Y = -scissor[0].Y*MatFinal._42 + MatFinal._22;
	m_ViewFrustum[2].m_Norm.Z = -scissor[0].Y*MatFinal._43 + MatFinal._23;
	m_ViewFrustum[2].m_Dist   = -scissor[0].Y*MatFinal._44 + MatFinal._24;

	// get the TOP plane
	m_ViewFrustum[3].m_Norm.X = scissor[1].Y*MatFinal._41 - MatFinal._21;
	m_ViewFrustum[3].m_Norm.Y = scissor[1].Y*MatFinal._42 - MatFinal._22;
	m_ViewFrustum[3].m_Norm.Z = scissor[1].Y*MatFinal._43 - MatFinal._23;
	m_ViewFrustum[3].m_Dist   = scissor[1].Y*MatFinal._44 - MatFinal._24;

	// get the FAR plane
	m_ViewFrustum[4].m_Norm.X = scissor[1].Z*MatFinal._41 - MatFinal._31;
	m_ViewFrustum[4].m_Norm.Y = scissor[1].Z*MatFinal._42 - MatFinal._32;
	m_ViewFrustum[4].m_Norm.Z = scissor[1].Z*MatFinal._43 - MatFinal._33;
	m_ViewFrustum[4].m_Dist   = scissor[1].Z*MatFinal._44 - MatFinal._34;

	// get the NEAR plane
	m_ViewFrustum[5].m_Norm.X = -scissor[0].Z*MatFinal._41 + MatFinal._31;
	m_ViewFrustum[5].m_Norm.Y = -scissor[0].Z*MatFinal._42 + MatFinal._32;
	m_ViewFrustum[5].m_Norm.Z = -scissor[0].Z*MatFinal._43 + MatFinal._33;
	m_ViewFrustum[5].m_Dist   = -scissor[0].Z*MatFinal._44 + MatFinal._34;

	for (size_t i = 0; i < 6; ++i)
		m_ViewFrustum[i].Normalize();
}

void CCamera::ClipFrustum(const CPlane& clipPlane)
{
	CPlane normClipPlane = clipPlane;
	normClipPlane.Normalize();
	m_ViewFrustum.AddPlane(normClipPlane);
}

void CCamera::SetViewPort(const SViewPort& viewport)
{
	m_ViewPort.m_X = viewport.m_X;
	m_ViewPort.m_Y = viewport.m_Y;
	m_ViewPort.m_Width = viewport.m_Width;
	m_ViewPort.m_Height = viewport.m_Height;
}

float CCamera::GetAspectRatio() const
{
	return static_cast<float>(m_ViewPort.m_Width) / static_cast<float>(m_ViewPort.m_Height);
}

void CCamera::GetViewQuad(float dist, Quad& quad) const
{
	if (m_ProjType == ProjectionType::CUSTOM)
	{
		const CMatrix3D invProjection = m_ProjMat.GetInverse();
		const std::array<CVector2D, 4> ndcCorners = {
			CVector2D{-1.0f, -1.0f}, CVector2D{1.0f, -1.0f},
			CVector2D{1.0f, 1.0f}, CVector2D{-1.0f, 1.0f}};
		for (size_t idx = 0; idx < 4; ++idx)
		{
			const CVector2D& corner = ndcCorners[idx];
			CVector4D nearCorner =
				invProjection.Transform(CVector4D(corner.X, corner.Y, -1.0f, 1.0f));
			nearCorner /= nearCorner.W;
			CVector4D farCorner =
				invProjection.Transform(CVector4D(corner.X, corner.Y, 1.0f, 1.0f));
			farCorner /= farCorner.W;
			const float t = (dist - nearCorner.Z) / (farCorner.Z - nearCorner.Z);
			const CVector4D quadCorner = nearCorner * (1.0 - t) + farCorner * t;
			quad[idx].X = quadCorner.X;
			quad[idx].Y = quadCorner.Y;
			quad[idx].Z = quadCorner.Z;
		}
		return;
	}

	const float y = m_ProjType == ProjectionType::PERSPECTIVE ? dist * tanf(m_FOV * 0.5f) : m_OrthoScale * 0.5f;
	const float x = y * GetAspectRatio();

	quad[0].X = -x;
	quad[0].Y = -y;
	quad[0].Z = dist;
	quad[1].X = x;
	quad[1].Y = -y;
	quad[1].Z = dist;
	quad[2].X = x;
	quad[2].Y = y;
	quad[2].Z = dist;
	quad[3].X = -x;
	quad[3].Y = y;
	quad[3].Z = dist;
}

void CCamera::BuildCameraRay(int px, int py, CVector3D& origin, CVector3D& dir) const
{
	ENSURE(m_ProjType == ProjectionType::PERSPECTIVE || m_ProjType == ProjectionType::ORTHO);

	// Coordinates relative to the camera plane.
	const float dx = static_cast<float>(px) / m_ViewPort.m_Width;
	const float dy = 1.0f - static_cast<float>(py) / m_ViewPort.m_Height;

	Quad points;
	GetViewQuad(m_FarPlane, points);

	// Transform from camera space to world space.
	for (CVector3D& point : points)
		point = m_Orientation.Transform(point);

	// Get world space position of mouse point at the far clipping plane.
	const CVector3D basisX = points[1] - points[0];
	const CVector3D basisY = points[3] - points[0];

	if (m_ProjType == ProjectionType::PERSPECTIVE)
	{
		// Build direction for the camera origin to the target point.
		origin = m_Orientation.GetTranslation();
		CVector3D targetPoint = points[0] + (basisX * dx) + (basisY * dy);
		dir = targetPoint - origin;
	}
	else if (m_ProjType == ProjectionType::ORTHO)
	{
		origin = m_Orientation.GetTranslation() + (basisX * (dx - 0.5f)) + (basisY * (dy - 0.5f));
		dir = m_Orientation.GetIn();
	}
	dir.Normalize();
}

void CCamera::GetScreenCoordinates(const CVector3D& world, float& x, float& y) const
{
	CMatrix3D transform = m_ProjMat * m_Orientation.GetInverse();

	CVector4D screenspace = transform.Transform(CVector4D(world.X, world.Y, world.Z, 1.0f));

	x = screenspace.X / screenspace.W;
	y = screenspace.Y / screenspace.W;
	x = (x + 1) * 0.5f * m_ViewPort.m_Width;
	y = (1 - y) * 0.5f * m_ViewPort.m_Height;
}

CVector3D CCamera::GetWorldCoordinates(int px, int py, bool aboveWater) const
{
	CHFTracer tracer(g_Game->GetWorld()->GetTerrain());
	int x, z;
	CVector3D origin, dir, delta, terrainPoint, waterPoint;

	BuildCameraRay(px, py, origin, dir);

	bool gotTerrain = tracer.RayIntersect(origin, dir, x, z, terrainPoint);

	if (!aboveWater)
	{
		if (gotTerrain)
			return terrainPoint;

		// Off the edge of the world?
		// Work out where it /would/ hit, if the map were extended out to infinity with average height.
		return GetWorldCoordinates(px, py, 50.0f);
	}

	CPlane plane;
	plane.Set(CVector3D(0.f, 1.f, 0.f),										// upwards normal
		CVector3D(0.f, g_Renderer.GetSceneRenderer().GetWaterManager().m_WaterHeight, 0.f));	// passes through water plane

	bool gotWater = plane.FindRayIntersection( origin, dir, &waterPoint );

	// Clamp the water intersection to within the map's bounds, so that
	// we'll always return a valid position on the map
	const ssize_t mapSize = g_Game->GetWorld()->GetTerrain().GetVerticesPerSide();
	if (gotWater)
	{
		waterPoint.X = Clamp<float>(waterPoint.X, 0.f, (mapSize - 1) * TERRAIN_TILE_SIZE);
		waterPoint.Z = Clamp<float>(waterPoint.Z, 0.f, (mapSize - 1) * TERRAIN_TILE_SIZE);
	}

	if (gotTerrain)
	{
		if (gotWater)
		{
			// Intersecting both heightmap and water plane; choose the closest of those
			if ((origin - terrainPoint).LengthSquared() < (origin - waterPoint).LengthSquared())
				return terrainPoint;
			else
				return waterPoint;
		}
		else
		{
			// Intersecting heightmap but parallel to water plane
			return terrainPoint;
		}
	}
	else
	{
		if (gotWater)
		{
			// Only intersecting water plane
			return waterPoint;
		}
		else
		{
			// Not intersecting terrain or water; just return 0,0,0.
			return CVector3D(0.f, 0.f, 0.f);
		}
	}

}

CVector3D CCamera::GetWorldCoordinates(int px, int py, float h) const
{
	CPlane plane;
	plane.Set(CVector3D(0.f, 1.f, 0.f), CVector3D(0.f, h, 0.f)); // upwards normal, passes through h

	CVector3D origin, dir, delta, currentTarget;

	BuildCameraRay(px, py, origin, dir);

	if (plane.FindRayIntersection(origin, dir, &currentTarget))
		return currentTarget;

	// No intersection with the infinite plane - nothing sensible can be returned,
	// so just choose an arbitrary point on the plane
	return CVector3D(0.f, h, 0.f);
}

CVector3D CCamera::GetFocus() const
{
	// Basically the same as GetWorldCoordinates

	CHFTracer tracer(g_Game->GetWorld()->GetTerrain());
	int x, z;

	CVector3D origin, dir, delta, terrainPoint, waterPoint;

	origin = m_Orientation.GetTranslation();
	dir = m_Orientation.GetIn();

	bool gotTerrain = tracer.RayIntersect(origin, dir, x, z, terrainPoint);

	CPlane plane;
	plane.Set(CVector3D(0.f, 1.f, 0.f),										// upwards normal
		CVector3D(0.f, g_Renderer.GetSceneRenderer().GetWaterManager().m_WaterHeight, 0.f));	// passes through water plane

	bool gotWater = plane.FindRayIntersection( origin, dir, &waterPoint );

	// Clamp the water intersection to within the map's bounds, so that
	// we'll always return a valid position on the map
	const ssize_t mapSize = g_Game->GetWorld()->GetTerrain().GetVerticesPerSide();
	if (gotWater)
	{
		waterPoint.X = Clamp<float>(waterPoint.X, 0.f, (mapSize - 1) * TERRAIN_TILE_SIZE);
		waterPoint.Z = Clamp<float>(waterPoint.Z, 0.f, (mapSize - 1) * TERRAIN_TILE_SIZE);
	}

	if (gotTerrain)
	{
		if (gotWater)
		{
			// Intersecting both heightmap and water plane; choose the closest of those
			if ((origin - terrainPoint).LengthSquared() < (origin - waterPoint).LengthSquared())
				return terrainPoint;
			else
				return waterPoint;
		}
		else
		{
			// Intersecting heightmap but parallel to water plane
			return terrainPoint;
		}
	}
	else
	{
		if (gotWater)
		{
			// Only intersecting water plane
			return waterPoint;
		}
		else
		{
			// Not intersecting terrain or water; just return 0,0,0.
			return CVector3D(0.f, 0.f, 0.f);
		}
	}
}

CBoundingBoxAligned CCamera::GetBoundsInViewPort(const CBoundingBoxAligned& boundigBox) const
{
	const CVector3D cameraPosition = GetOrientation().GetTranslation();
	if (boundigBox.IsPointInside(cameraPosition))
		return CBoundingBoxAligned(CVector3D(-1.0f, -1.0f, 0.0f), CVector3D(1.0f, 1.0f, 0.0f));

	const CMatrix3D viewProjection = GetViewProjection();
	CBoundingBoxAligned viewPortBounds;
#define ADD_VISIBLE_POINT_TO_VIEWBOUNDS(POSITION) STMT( \
		CVector4D v = viewProjection.Transform(CVector4D((POSITION).X, (POSITION).Y, (POSITION).Z, 1.0f)); \
		if (v.W != 0.0f) \
			viewPortBounds += CVector3D(v.X, v.Y, v.Z) * (1.0f / v.W); )

	std::array<CVector3D, 8> worldPositions;
	std::array<bool, 8> isBehindNearPlane;
	const CVector3D lookDirection = GetOrientation().GetIn();
	// Check corners.
	for (size_t idx = 0; idx < 8; ++idx)
	{
		worldPositions[idx] = CVector3D(boundigBox[(idx >> 0) & 0x1].X, boundigBox[(idx >> 1) & 0x1].Y, boundigBox[(idx >> 2) & 0x1].Z);
		isBehindNearPlane[idx] = lookDirection.Dot(worldPositions[idx]) < lookDirection.Dot(cameraPosition) + GetNearPlane();
		if (!isBehindNearPlane[idx])
			ADD_VISIBLE_POINT_TO_VIEWBOUNDS(worldPositions[idx]);
	}
	// Check edges for intersections with the near plane.
	for (size_t idxBegin = 0; idxBegin < 8; ++idxBegin)
		for (size_t nextComponent = 0; nextComponent < 3; ++nextComponent)
		{
			const size_t idxEnd = idxBegin | (1u << nextComponent);
			if (idxBegin == idxEnd || isBehindNearPlane[idxBegin] == isBehindNearPlane[idxEnd])
				continue;
			CVector3D intersection;
			// Intersect the segment with the near plane.
			if (!m_ViewFrustum[5].FindLineSegIntersection(worldPositions[idxBegin], worldPositions[idxEnd], &intersection))
				continue;
			ADD_VISIBLE_POINT_TO_VIEWBOUNDS(intersection);
		}
#undef ADD_VISIBLE_POINT_TO_VIEWBOUNDS
	if (viewPortBounds[0].X >= 1.0f || viewPortBounds[1].X <= -1.0f || viewPortBounds[0].Y >= 1.0f || viewPortBounds[1].Y <= -1.0f)
		return CBoundingBoxAligned{};
	return viewPortBounds;
}

void CCamera::LookAt(const CVector3D& camera, const CVector3D& focus, const CVector3D& up)
{
	CVector3D delta = focus - camera;
	LookAlong(camera, delta, up);
}

void CCamera::LookAlong(const CVector3D& camera, CVector3D orientation, CVector3D up)
{
	orientation.Normalize();
	up.Normalize();
	const CVector3D s = orientation.Cross(up);
	up = s.Cross(orientation);

	m_Orientation._11 = -s.X;	m_Orientation._12 = up.X;	m_Orientation._13 = orientation.X;	m_Orientation._14 = camera.X;
	m_Orientation._21 = -s.Y;	m_Orientation._22 = up.Y;	m_Orientation._23 = orientation.Y;	m_Orientation._24 = camera.Y;
	m_Orientation._31 = -s.Z;	m_Orientation._32 = up.Z;	m_Orientation._33 = orientation.Z;	m_Orientation._34 = camera.Z;
	m_Orientation._41 = 0.0f;	m_Orientation._42 = 0.0f;	m_Orientation._43 = 0.0f;			m_Orientation._44 = 1.0f;
}
