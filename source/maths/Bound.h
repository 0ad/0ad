/* Copyright (C) 2011 Wildfire Games.
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
 * Axis-aligned bounding box
 */

#ifndef INCLUDED_BOUND
#define INCLUDED_BOUND

// necessary includes
#include "Vector3D.h"

class CFrustum;
class CMatrix3D;

///////////////////////////////////////////////////////////////////////////////
// CBound: basic axis aligned bounding box class
class CBound
{
public:
	CBound() { SetEmpty(); }
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

	/**
	 * Expand the bounding box by the given amount in every direction.
	 */
	void Expand(float amount);

	/**
	 * IntersectFrustumConservative: Approximate the intersection of this bounds object
	 * with the given frustum. The bounds object is overwritten with the results.
	 *
	 * The approximation is conservative in the sense that the result will always contain
	 * the actual intersection, but it may be larger than the intersection itself.
	 * The result will always be fully contained within the original bounds.
	 *
	 * @note While not in the spirit of this function's purpose, a no-op would be a correct
	 * implementation of this function.
	 *
	 * @param frustum the frustum to intersect with
	 */
	void IntersectFrustumConservative(const CFrustum& frustum);

	/**
	 * Render: Render the surfaces of the bound object as polygons.
	 */
	void Render() const;

private:
	CVector3D m_Data[2];
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
