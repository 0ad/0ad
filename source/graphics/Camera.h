//***********************************************************
//
// Name:		Camera.H
// Last Update: 24/2/02
// Author:		Poya Manouchehri
//
// Description: CCamera holds a view and a projection matrix.
//				It also has a frustum which can be used to
//				cull objects for rendering.
//
//***********************************************************

#ifndef CAMERA_H
#define CAMERA_H

#include "Frustum.h"
#include "Matrix3D.h"

//view port
struct SViewPort
{
	unsigned int m_X;
	unsigned int m_Y;
	unsigned int m_Width;
	unsigned int m_Height;
};


class CCamera
{
	public:
		CCamera ();
		~CCamera ();
		
		//Methods for projection
		void SetProjection (CMatrix3D *proj) { m_ProjMat = *proj; }
		void SetProjection (float nearp, float farp, float fov);
		CMatrix3D GetProjection () { return m_ProjMat; }

		//Updates the frustum planes. Should be called
		//everytime the view or projection matrices are
		//altered.
		void UpdateFrustum ();
		CFrustum GetFustum () { return m_ViewFrustum; }

		void SetViewPort (SViewPort *viewport);
		SViewPort GetViewPort () { return m_ViewPort; }

		//getters
		float GetNearPlane() const { return m_NearPlane; }
		float GetFarPlane() const { return m_FarPlane; }
		float GetFOV() const { return m_FOV; }

		// calculate and return the position of the 8 points of the frustum in world space
		void GetFrustumPoints(CVector3D pts[8]) const;

		// return four points in camera space at given distance from camera
		void GetCameraPlanePoints(float dist,CVector3D pts[4]) const;

	public:
		//This is the orientation matrix. The inverse of this
		//is the view matrix
		CMatrix3D		m_Orientation;

	private:
		//keep the projection matrix private
		//so we can't fiddle with it.
		CMatrix3D		m_ProjMat;

		float			m_NearPlane;
		float			m_FarPlane;
		float			m_FOV;
		SViewPort		m_ViewPort;

		CFrustum		m_ViewFrustum;
};

#endif
