///////////////////////////////////////////////////////////////////////////////
//
// Name:		Bound.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _BOUND_H
#define _BOUND_H

// necessary includes
#include "Vector3D.h"
#include "Matrix3D.h"

///////////////////////////////////////////////////////////////////////////////
// CBound: basic axis aligned bounding box class
class CBound
{
public:
	CBound() {}
	CBound(const CVector3D& min,const CVector3D& max) {
		m_Data[0]=min; m_Data[1]=max;
	}

	void Transform(const CMatrix3D& m,CBound& result) const;

	CVector3D& operator[](int index) {	return m_Data[index]; }
	const CVector3D& operator[](int index) const { return m_Data[index]; }

	void SetEmpty();
	bool IsEmpty();

	CBound& operator+=(const CBound& b);
	CBound& operator+=(const CVector3D& pt);

	bool RayIntersect(const CVector3D& origin,const CVector3D& dir,float& tmin,float& tmax) const;

	// return the volume of this bounding box
	float GetVolume() const {
		CVector3D v=m_Data[1]-m_Data[0];
		return v.X*v.Y*v.Z;
	}
	
	// return the centre of this bounding box
	void GetCentre(CVector3D& centre) const {
		centre=(m_Data[0]+m_Data[1])*0.5f;
	}

private:
	CVector3D m_Data[2];
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
