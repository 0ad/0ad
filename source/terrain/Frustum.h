//***********************************************************
//
// Name:		Frustum.h
// Last Update: 24/2/02
// Author:		Poya Manouchehri
//
// Description: CFrustum is a collection of planes which define
//				a viewing space. Usually associated with the
//				camera, there are 6 planes which define the
//				view pyramid. But we allow more planes per
//				frustum which maybe used for portal rendering,
//				where a portal may have 3 or more edges.
//
//***********************************************************

#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "Plane.h"

//10 planes should be enough
#define MAX_NUM_FRUSTUM_PLANES		(10)


struct SBoundingBox
{
	CVector3D m_BoxMin;
	CVector3D m_BoxMax;
};

class CFrustum
{
	public:
		CFrustum ();
		~CFrustum ();

		//Set the number of planes to use for
		//calculations. This is clipped to 
		//[0,MAX_NUM_FRUSTUM_PLANES]
		void SetNumPlanes (int num);

		//The following methods return true if the shape is
		//partially or completely in front of the frustum planes
		bool IsPointVisible (CVector3D &point);
		bool IsSphereVisible (CVector3D &center, float radius);
		bool IsBoxVisible (CVector3D &position, SBoundingBox &bounds);

	public:
		//make the planes public for ease of use
		CPlane m_aPlanes[MAX_NUM_FRUSTUM_PLANES];

	private:
		int m_NumPlanes;
};
		
#endif