//***********************************************************
//
// Name:		Frustum.Cpp
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

#include "Frustum.h"

CFrustum::CFrustum ()
{
	m_NumPlanes = 0;
}

CFrustum::~CFrustum ()
{
}

void CFrustum::SetNumPlanes (int num)
{
	m_NumPlanes = num;

	//clip it
	if (m_NumPlanes >= MAX_NUM_FRUSTUM_PLANES)
		m_NumPlanes = MAX_NUM_FRUSTUM_PLANES-1;
	else if (m_NumPlanes < 0)
		m_NumPlanes	= 0;
}

bool CFrustum::IsPointVisible (const CVector3D &point) const
{
	PLANESIDE Side;

	for (int i=0; i<m_NumPlanes; i++)
	{
		Side = m_aPlanes[i].ClassifyPoint (point);

		if (Side == PS_BACK)
			return false;
	}

	return true;
}

bool CFrustum::IsSphereVisible (const CVector3D &center, float radius) const
{
	for (int i=0; i<m_NumPlanes; i++)
	{
		float Dist = m_aPlanes[i].DistanceToPlane (center);
		
		//is it behind the plane
		if (Dist < 0)
		{
			//if non of it falls in front its outside the
			//frustum
			if (-Dist > radius)
				return false;
		}
	}

	return true;
}


bool CFrustum::IsBoxVisible (const CVector3D &position,const CBound &bounds) const
{
	//basically for every plane we calculate the furthust point
	//in the box to that plane. If that point is beyond the plane
	//then the box is not visible
	CVector3D FarPoint;
	PLANESIDE Side;
	CVector3D Min = position+bounds[0];
	CVector3D Max = position+bounds[1];

	for (int i=0; i<m_NumPlanes; i++)
	{
		if (m_aPlanes[i].m_Norm.X > 0.0f)
		{
			if (m_aPlanes[i].m_Norm.Y > 0.0f)
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Max.X; FarPoint.Y = Max.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Max.X;	FarPoint.Y = Max.Y; FarPoint.Z = Min.Z;
				}
			}
			else
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Max.X; FarPoint.Y = Min.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Max.X;	FarPoint.Y = Min.Y; FarPoint.Z = Min.Z;
				}
			}
		}
		else
		{
			if (m_aPlanes[i].m_Norm.Y > 0.0f)
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Min.X; FarPoint.Y = Max.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Min.X;	FarPoint.Y = Max.Y; FarPoint.Z = Min.Z;
				}
			}
			else
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Min.X; FarPoint.Y = Min.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Min.X;	FarPoint.Y = Min.Y; FarPoint.Z = Min.Z;
				}
			}
		}
		
		Side = m_aPlanes[i].ClassifyPoint (FarPoint);

		if (Side == PS_BACK)
			return false;
	}

	return true;
}

