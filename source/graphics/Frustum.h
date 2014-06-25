/* Copyright (C) 2009 Wildfire Games.
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
 * CFrustum is a collection of planes which define a viewing space.
 */

/*
Usually associated with the camera, there are 6 planes which define the
view pyramid. But we allow more planes per frustum which may be used for
portal rendering, where a portal may have 3 or more edges.
*/

#ifndef INCLUDED_FRUSTUM
#define INCLUDED_FRUSTUM

#include "maths/Plane.h"

//10 planes should be enough
#define MAX_NUM_FRUSTUM_PLANES		(10)

class CBoundingBoxAligned;
class CMatrix3D;

class CFrustum
{
public:
	CFrustum ();
	~CFrustum ();

	//Set the number of planes to use for
	//calculations. This is clipped to
	//[0,MAX_NUM_FRUSTUM_PLANES]
	void SetNumPlanes (size_t num);

	size_t GetNumPlanes() const { return m_NumPlanes; }

	void AddPlane (const CPlane& plane);

	void Transform(CMatrix3D& m);

	//The following methods return true if the shape is
	//partially or completely in front of the frustum planes
	bool IsPointVisible (const CVector3D &point) const;
	bool DoesSegmentIntersect(const CVector3D& start, const CVector3D &end);
	bool IsSphereVisible (const CVector3D &center, float radius) const;
	bool IsBoxVisible (const CVector3D &position,const CBoundingBoxAligned &bounds) const;

	CPlane& operator[](size_t idx) { return m_aPlanes[idx]; }
	const CPlane& operator[](size_t idx) const { return m_aPlanes[idx]; }

public:
	//make the planes public for ease of use
	CPlane m_aPlanes[MAX_NUM_FRUSTUM_PLANES];

private:
	size_t m_NumPlanes;
};

#endif
