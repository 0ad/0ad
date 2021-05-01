/* Copyright (C) 2021 Wildfire Games.
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

/*
 * CCamera holds a view and a projection matrix. It also has a frustum
 * which can be used to cull objects for rendering.
 */

#ifndef INCLUDED_CAMERA
#define INCLUDED_CAMERA

#include "maths/BoundingBoxAligned.h"
#include "maths/Frustum.h"
#include "maths/Matrix3D.h"

#include <array>

// view port
struct SViewPort
{
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;
};

class CCamera
{
	public:
		// Represents camera viewport or frustum side in 3D space.
		using Quad = std::array<CVector3D, 4>;

		enum class ProjectionType
		{
			CUSTOM,
			ORTHO,
			PERSPECTIVE,
		};

		CCamera();
		~CCamera();

		CMatrix3D& GetProjection() { return m_ProjMat; }
		const CMatrix3D& GetProjection() const { return m_ProjMat; }
		CMatrix3D GetViewProjection() const { return m_ProjMat * m_Orientation.GetInverse(); }
		void SetProjection(const CMatrix3D& matrix);
		void SetProjectionFromCamera(const CCamera& camera);
		void SetOrthoProjection(float nearp, float farp, float scale);
		void SetPerspectiveProjection(float nearp, float farp, float fov);
		ProjectionType GetProjectionType() const { return m_ProjType; }

		CMatrix3D& GetOrientation() { return m_Orientation; }
		const CMatrix3D& GetOrientation() const { return m_Orientation; }

		// Updates the frustum planes. Should be called
		// everytime the view or projection matrices are
		// altered.
		void UpdateFrustum(const CBoundingBoxAligned& scissor = CBoundingBoxAligned(CVector3D(-1.0f, -1.0f, -1.0f), CVector3D(1.0f, 1.0f, 1.0f)));
		void ClipFrustum(const CPlane& clipPlane);
		const CFrustum& GetFrustum() const { return m_ViewFrustum; }

		void SetViewPort(const SViewPort& viewport);
		const SViewPort& GetViewPort() const { return m_ViewPort; }
		float GetAspectRatio() const;

		float GetNearPlane() const { return m_NearPlane; }
		float GetFarPlane() const { return m_FarPlane; }
		float GetFOV() const { return m_FOV; }
		float GetOrthoScale() const { return m_OrthoScale; }

		// Returns a quad of view in camera space at given distance from camera.
		void GetViewQuad(float dist, Quad& quad) const;

		// Builds a ray passing through the screen coordinate (px, py), calculates
		// origin and direction of the ray.
		void BuildCameraRay(int px, int py, CVector3D& origin, CVector3D& dir) const;

		// General helpers that seem to fit here

		// Get the screen-space coordinates corresponding to a given world-space position
		void GetScreenCoordinates(const CVector3D& world, float& x, float& y) const;

		// Get the point on the terrain corresponding to pixel (px,py) (or the mouse coordinates)
		// The aboveWater parameter determines whether we want to stop at the water plane or also get underwater points
		CVector3D GetWorldCoordinates(int px, int py, bool aboveWater=false) const;
		// Get the point on the plane at height h corresponding to pixel (px,py)
		CVector3D GetWorldCoordinates(int px, int py, float h) const;
		// Get the point on the terrain (or water plane) the camera is pointing towards
		CVector3D GetFocus() const;

		CBoundingBoxAligned GetBoundsInViewPort(const CBoundingBoxAligned& boundigBox) const;

		// Build an orientation matrix from camera position, camera focus point, and up-vector
		void LookAt(const CVector3D& camera, const CVector3D& focus, const CVector3D& up);

		// Build an orientation matrix from camera position, camera orientation, and up-vector
		void LookAlong(const CVector3D& camera, CVector3D orientation, CVector3D up);

	public:
		// This is the orientation matrix. The inverse of this
		// is the view matrix
		CMatrix3D m_Orientation;

	private:
		CMatrix3D m_ProjMat;
		ProjectionType m_ProjType = ProjectionType::CUSTOM;

		float m_NearPlane = 0.0f;
		float m_FarPlane = 0.0f;
		union
		{
			float m_FOV;
			float m_OrthoScale;
		};
		SViewPort m_ViewPort;

		CFrustum m_ViewFrustum;
};

#endif // INCLUDED_CAMERA
