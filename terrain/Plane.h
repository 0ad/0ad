//***********************************************************
//
// Name:		Plane.H
// Last Update:	17/2/02
// Author:		Poya Manouchehri
//
// Description: A Plane in R3 and several utility methods. 
//				Note that the format used for the plane
//				equation is Ax + By + Cz + D = 0, where 
//				<A,B,C> is the normal vector.
//
//***********************************************************

#ifndef PLANE_H
#define PLANE_H

#include "Vector3D.H"

enum PLANESIDE
{
	PS_FRONT,
	PS_BACK,
	PS_ON
};

class CPlane
{
	public:
		CPlane ();

		//sets the plane equation from 3 points on that plane
		void Set (CVector3D &p1, CVector3D &p2, CVector3D &p3);

		//sets the plane equation from a normal and a point on 
		//that plane
		void Set (CVector3D &norm, CVector3D &point);

		//normalizes the plane equation
		void Normalize ();

		//returns the side of the plane on which this point
		//lies.
		PLANESIDE ClassifyPoint (CVector3D &point);

		//solves the plane equation for a particular point
		float DistanceToPlane (CVector3D &point);

		//calculates the intersection point of a line with this
		//plane. Returns false if there is no intersection
		bool FindLineSegIntersection (CVector3D &start, CVector3D &end, CVector3D *intsect);
		bool FindRayIntersection (CVector3D &start, CVector3D &direction, CVector3D *intsect);

	public:
		CVector3D m_Norm;	//normal vector of the plane
		float m_Dist;		//Plane distance (ie D in the plane eq.)
};

#endif