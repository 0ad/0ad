//***********************************************************
//
// Name:		Plane.Cpp
// Last Update:	17/2/02
// Author:		Poya Manouchehri
//
// Description: A Plane in R3 and several utility methods. 
//				Note that the format used for the plane
//				equation is Ax + By + Cz + D = 0, where 
//				<A,B,C> is the normal vector.
//
//***********************************************************

#include "Plane.h"

CPlane::CPlane ()
{
	m_Norm.Clear ();
	m_Dist = 0.0f;
}

//sets the plane equation from 3 points on that plane
void CPlane::Set (CVector3D &p1, CVector3D &p2, CVector3D &p3)
{
	CVector3D D1, D2;
	CVector3D Norm;

	//calculate two vectors on the surface of the plane
	D1 = p2-p1;
	D2 = p3-p1;

	//cross multiply gives normal
	Norm = D2.Cross(D1);

	Set (Norm, p1);
}

//sets the plane equation from a normal and a point on 
//that plane
void CPlane::Set (CVector3D &norm, CVector3D &point)
{
	m_Norm = norm;

	m_Dist = - (norm.X * point.X +
				norm.Y * point.Y +
				norm.Z * point.Z);

//	Normalize ();
}

//normalizes the plane equation
void CPlane::Normalize ()
{
	float Scale;

	Scale = 1.0f/m_Norm.GetLength ();

	m_Norm.X *= Scale;
	m_Norm.Y *= Scale;
	m_Norm.Z *= Scale;
	m_Dist *= Scale;
}

//returns the side of the plane on which this point
//lies.
PLANESIDE CPlane::ClassifyPoint (CVector3D &point)
{
	float Dist;

	Dist = m_Norm.X * point.X +
		   m_Norm.Y * point.Y +
		   m_Norm.Z * point.Z +
		   m_Dist;

	if (Dist > 0.0f)
		return PS_FRONT;
	else if (Dist < 0.0f)
		return PS_BACK;

	return PS_ON;
}

//solves the plane equation for a particular point
float CPlane::DistanceToPlane (CVector3D &point)
{
	float Dist;

	Dist = m_Norm.X * point.X +
		   m_Norm.Y * point.Y +
		   m_Norm.Z * point.Z +
		   m_Dist;

	return Dist;
}

//calculates the intersection point of a line with this
//plane. Returns false if there is no intersection
bool CPlane::FindLineSegIntersection (CVector3D &start, CVector3D &end, CVector3D *intsect)
{
	PLANESIDE StartS, EndS;
	CVector3D Dir;
	float Length;
	
	//work out where each point is
	StartS = ClassifyPoint (start);
	EndS = ClassifyPoint (end);

	//if they are not on opposite sides of the plane return false
	if (StartS == EndS)
		return false;

	//work out a normalized vector in the direction start to end
	Dir = end - start;
	Dir.Normalize ();

	//a bit of algebra to work out how much we need to scale
	//this direction vector to get to the plane
	Length = -m_Norm.Dot(start)/m_Norm.Dot(Dir);

	//scale it by this amount
	Dir *= Length;

	//workout actual position vector of impact
	*intsect = start + Dir;

	return true;
}

bool CPlane::FindRayIntersection (CVector3D &start, CVector3D &direction, CVector3D *intsect)
{
	float dot = m_Norm.Dot (direction);
	if (dot == 0.0f)
		return false;

	CVector3D a;
	*intsect = start - (direction * (DistanceToPlane (start)/dot));
	return true;
}