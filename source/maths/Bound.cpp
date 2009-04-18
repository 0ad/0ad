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

/**
 * =========================================================================
 * File        : Bound.cpp
 * Project     : 0 A.D.
 * Description : Axis-aligned bounding box
 * =========================================================================
 */

#include "precompiled.h"

// necessary includes

#include "lib/ogl.h"

#include <float.h>
#include "Bound.h"
#include "graphics/Frustum.h"
#include "Brush.h"


///////////////////////////////////////////////////////////////////////////////
// operator+=: extend this bound to include given bound
CBound& CBound::operator+=(const CBound& b)
{
#define CMPT(c) \
	if (b[0].c < m_Data[0].c) m_Data[0].c = b[0].c; \
	if (b[1].c > m_Data[1].c) m_Data[1].c = b[1].c
	CMPT(X);
	CMPT(Y);
	CMPT(Z);
#undef CMPT

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// operator+=: extend this bound to include given point
CBound& CBound::operator+=(const CVector3D& pt)
{
#define CMPT(c) \
	if (pt.c < m_Data[0].c) m_Data[0].c = pt.c; \
	if (pt.c > m_Data[1].c) m_Data[1].c = pt.c
	CMPT(X);
	CMPT(Y);
	CMPT(Z);
#undef CMPT

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// RayIntersect: intersect ray with this bound; return true
// if ray hits (and store entry and exit times), or false
// otherwise
// note: incoming ray direction must be normalised
bool CBound::RayIntersect(const CVector3D& origin,const CVector3D& dir,
			float& tmin,float& tmax) const
{
	float t1,t2;
	float tnear,tfar;

	if (dir[0]==0) {
		if (origin[0]<m_Data[0][0] || origin[0]>m_Data[1][0])
			return false;
		else {
			tnear=(float) -FLT_MAX;
			tfar=(float) FLT_MAX;
		}
	} else {
		t1=(m_Data[0][0]-origin[0])/dir[0];
		t2=(m_Data[1][0]-origin[0])/dir[0];

		if (dir[0]<0) {
			tnear = t2;
			tfar = t1;
		} else {
			tnear = t1;
			tfar = t2;
		}

		if (tfar<0)
			return false;
	}

	if (dir[1]==0 && (origin[1]<m_Data[0][1] || origin[1]>m_Data[1][1]))
		return false;
	else {
		t1=(m_Data[0][1]-origin[1])/dir[1];
		t2=(m_Data[1][1]-origin[1])/dir[1];

		if (dir[1]<0) {
			if (t2>tnear)
				tnear = t2;
			if (t1<tfar)
				tfar = t1;
		} else {
			if (t1>tnear)
				tnear = t1;
			if (t2<tfar)
				tfar = t2;
		}

		if (tnear>tfar || tfar<0)
			return false;
	}

	if (dir[2]==0 && (origin[2]<m_Data[0][2] || origin[2]>m_Data[1][2]))
		return false;
	else {
		t1=(m_Data[0][2]-origin[2])/dir[2];
		t2=(m_Data[1][2]-origin[2])/dir[2];

		if (dir[2]<0) {
			if (t2>tnear)
				tnear = t2;
			if (t1<tfar)
				tfar = t1;
		} else {
			if (t1>tnear)
				tnear = t1;
			if (t2<tfar)
				tfar = t2;
		}

		if (tnear>tfar || tfar<0)
		return false;
	}

	tmin=tnear;
	tmax=tfar;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SetEmpty: initialise this bound as empty
void CBound::SetEmpty()
{
	m_Data[0]=CVector3D( FLT_MAX, FLT_MAX, FLT_MAX);
	m_Data[1]=CVector3D(-FLT_MAX,-FLT_MAX,-FLT_MAX);
}

///////////////////////////////////////////////////////////////////////////////
// IsEmpty: tests whether this bound is empty
bool CBound::IsEmpty()
{
	return (m_Data[0].X ==  FLT_MAX && m_Data[0].Y ==  FLT_MAX && m_Data[0].Z ==  FLT_MAX
	     && m_Data[1].X == -FLT_MAX && m_Data[1].Y == -FLT_MAX && m_Data[1].Z == -FLT_MAX);
}

///////////////////////////////////////////////////////////////////////////////
// Transform: transform this bound by given matrix; return transformed bound
// in 'result' parameter - slightly modified version of code in Graphic Gems
// (can't remember which one it was, though)
void CBound::Transform(const CMatrix3D& m,CBound& result) const
{
	debug_assert(this!=&result);

	for (int i=0;i<3;++i) {
		// handle translation
		result[0][i]=result[1][i]=m(i,3);

		// Now find the extreme points by considering the product of the
		// min and max with each component of matrix
		for(int j=0;j<3;j++) {
			float a=m(i,j)*m_Data[0][j];
			float b=m(i,j)*m_Data[1][j];

			if (a<b) {
				result[0][i]+=a;
				result[1][i]+=b;
			} else {
				result[0][i]+=b;
				result[1][i]+=a;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Intersect with the given frustum in a conservative manner
void CBound::IntersectFrustumConservative(const CFrustum& frustum)
{
	CBrush brush(*this);
	CBrush buf;

	brush.Intersect(frustum, buf);

	buf.Bounds(*this);
}


///////////////////////////////////////////////////////////////////////////////
// Render the bounding box
void CBound::Render()
{
	glBegin(GL_QUADS);
		glVertex3f(m_Data[0].X, m_Data[0].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[0].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[1].Y, m_Data[0].Z);
		glVertex3f(m_Data[0].X, m_Data[1].Y, m_Data[0].Z);

		glVertex3f(m_Data[0].X, m_Data[0].Y, m_Data[0].Z);
		glVertex3f(m_Data[0].X, m_Data[1].Y, m_Data[0].Z);
		glVertex3f(m_Data[0].X, m_Data[1].Y, m_Data[1].Z);
		glVertex3f(m_Data[0].X, m_Data[0].Y, m_Data[1].Z);

		glVertex3f(m_Data[0].X, m_Data[0].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[0].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[0].Y, m_Data[1].Z);
		glVertex3f(m_Data[0].X, m_Data[0].Y, m_Data[1].Z);

		glVertex3f(m_Data[0].X, m_Data[0].Y, m_Data[1].Z);
		glVertex3f(m_Data[1].X, m_Data[0].Y, m_Data[1].Z);
		glVertex3f(m_Data[1].X, m_Data[1].Y, m_Data[1].Z);
		glVertex3f(m_Data[0].X, m_Data[1].Y, m_Data[1].Z);

		glVertex3f(m_Data[1].X, m_Data[0].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[1].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[1].Y, m_Data[1].Z);
		glVertex3f(m_Data[1].X, m_Data[0].Y, m_Data[1].Z);

		glVertex3f(m_Data[0].X, m_Data[1].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[1].Y, m_Data[0].Z);
		glVertex3f(m_Data[1].X, m_Data[1].Y, m_Data[1].Z);
		glVertex3f(m_Data[0].X, m_Data[1].Y, m_Data[1].Z);
	glEnd();
}
