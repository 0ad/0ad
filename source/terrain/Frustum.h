//***********************************************************
//
// Name:		Frustum.H
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
#include "Bound.h"

//10 planes should be enough
#define MAX_NUM_FRUSTUM_PLANES		(10)


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
		bool IsPointVisible (const CVector3D &point) const;
		bool IsSphereVisible (const CVector3D &center, float radius) const;
		bool IsBoxVisible (const CVector3D &position,const CBound &bounds) const;

	public:
		//make the planes public for ease of use
		CPlane m_aPlanes[MAX_NUM_FRUSTUM_PLANES];

	private:
		int m_NumPlanes;
};
		
#endif
