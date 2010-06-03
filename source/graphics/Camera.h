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

/*
 * CCamera holds a view and a projection matrix. It also has a frustum
 * which can be used to cull objects for rendering.
 */

#ifndef INCLUDED_CAMERA
#define INCLUDED_CAMERA

#include "Frustum.h"
#include "maths/Matrix3D.h"

extern int g_mouse_x, g_mouse_y;

// view port
struct SViewPort
{
	size_t m_X;
	size_t m_Y;
	size_t m_Width;
	size_t m_Height;
};


class CCamera
{
	public:
		CCamera ();
		~CCamera ();
		
		// Methods for projection
		void SetProjection (CMatrix3D *proj) { m_ProjMat = *proj; }
		void SetProjection (float nearp, float farp, float fov);
		void SetProjectionTile (int tiles, int tile_x, int tile_y);
		CMatrix3D& GetProjection () { return m_ProjMat; }
		const CMatrix3D& GetProjection () const { return m_ProjMat; }

		// Updates the frustum planes. Should be called
		// everytime the view or projection matrices are
		// altered.
		void UpdateFrustum ();
		const CFrustum& GetFrustum () { return m_ViewFrustum; }

		void SetViewPort (const SViewPort& viewport);
		const SViewPort& GetViewPort () const { return m_ViewPort; }

		// getters
		float GetNearPlane() const { return m_NearPlane; }
		float GetFarPlane() const { return m_FarPlane; }
		float GetFOV() const { return m_FOV; }

		// calculate and return the position of the 8 points of the frustum in world space
		void GetFrustumPoints(CVector3D pts[8]) const;

		// return four points in camera space at given distance from camera
		void GetCameraPlanePoints(float dist,CVector3D pts[4]) const;

		// Build a ray passing through the screen coordinate (px, py) and the camera
		/////////////////////////////////////////////////////////////////////////////////////////
		// BuildCameraRay: calculate origin and ray direction of a ray through
		// the pixel (px,py) on the screen
		void BuildCameraRay(int px, int py, CVector3D& origin, CVector3D& dir);
		// BuildCameraRay: as previous, using global mouse position
		void BuildCameraRay(CVector3D& origin, CVector3D& dir)
		{
			BuildCameraRay(g_mouse_x, g_mouse_y, origin, dir);
		}

		// General helpers that seem to fit here

		// Get the screen-space coordinates corresponding to a given world-space position 
		void GetScreenCoordinates(const CVector3D& world, float& x, float& y) const;

		// Get the point on the terrain corresponding to pixel (px,py) (or the mouse coordinates)
		// The aboveWater parameter determines whether we want to stop at the water plane or also get underwater points
		CVector3D GetWorldCoordinates(int px, int py, bool aboveWater=false);
		CVector3D GetWorldCoordinates(bool aboveWater=false) 
			{ return GetWorldCoordinates(g_mouse_x, g_mouse_y, aboveWater); }
		// Get the point on the plane at height h corresponding to pixel (px,py)
		CVector3D GetWorldCoordinates(int px, int py, float h);
		// Get the point on the terrain the camera is pointing towards
		CVector3D GetFocus();

		// Build an orientation matrix from camera position, camera focus point, and up-vector
		void LookAt(const CVector3D& camera, const CVector3D& orientation, const CVector3D& up);

		// Build an orientation matrix from camera position, camera orientation, and up-vector
		void LookAlong(CVector3D camera, CVector3D focus, CVector3D up);

		/**
		 * Render: Renders the camera's frustum in world space.
		 * The caller should set the color using glColorXy before calling Render.
		 * 
		 * @param intermediates determines how many intermediate distance planes should
		 * be hinted at between the near and far planes
		 */
		void Render(int intermediates = 0) const;
		
	public:
		// This is the orientation matrix. The inverse of this
		// is the view matrix
		CMatrix3D		m_Orientation;
		
		// Should not be tweaked externally if possible
		CMatrix3D		m_ProjMat;

	private:
		float			m_NearPlane;
		float			m_FarPlane;
		float			m_FOV;
		SViewPort		m_ViewPort;

		CFrustum		m_ViewFrustum;
};

#endif
