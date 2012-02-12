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

#include "precompiled.h"

#include "BoundingBoxAligned.h"

#include "lib/ogl.h"

#include <float.h>

#include "graphics/Frustum.h"
#include "maths/BoundingBoxOriented.h"
#include "maths/Brush.h"
#include "maths/Matrix3D.h"

const CBoundingBoxAligned CBoundingBoxAligned::EMPTY = CBoundingBoxAligned(); // initializes to an empty bound

///////////////////////////////////////////////////////////////////////////////
// RayIntersect: intersect ray with this bound; return true
// if ray hits (and store entry and exit times), or false
// otherwise
// note: incoming ray direction must be normalised
bool CBoundingBoxAligned::RayIntersect(const CVector3D& origin,const CVector3D& dir,
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
void CBoundingBoxAligned::SetEmpty()
{
	m_Data[0]=CVector3D( FLT_MAX, FLT_MAX, FLT_MAX);
	m_Data[1]=CVector3D(-FLT_MAX,-FLT_MAX,-FLT_MAX);
}

///////////////////////////////////////////////////////////////////////////////
// IsEmpty: tests whether this bound is empty
bool CBoundingBoxAligned::IsEmpty() const
{
	return (m_Data[0].X ==  FLT_MAX && m_Data[0].Y ==  FLT_MAX && m_Data[0].Z ==  FLT_MAX
	     && m_Data[1].X == -FLT_MAX && m_Data[1].Y == -FLT_MAX && m_Data[1].Z == -FLT_MAX);
}

///////////////////////////////////////////////////////////////////////////////
// Transform: transform this bound by given matrix; return transformed bound
// in 'result' parameter - slightly modified version of code in Graphic Gems
// (can't remember which one it was, though)
void CBoundingBoxAligned::Transform(const CMatrix3D& m, CBoundingBoxAligned& result) const
{
	ENSURE(this!=&result);

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

void CBoundingBoxAligned::Transform(const CMatrix3D& transform, CBoundingBoxOriented& result) const
{
	// The idea is this: compute the corners of this bounding box, transform them according to the specified matrix,
	// then derive the box center, orientation vectors, and half-sizes.
	// TODO: this implementation can be further optimized; see Philip's comments on http://trac.wildfiregames.com/ticket/914
	const CVector3D& pMin = m_Data[0];
	const CVector3D& pMax = m_Data[1];

	// Find the corners of these bounds. We only need some of the corners to derive the information we need, so let's 
	// not actually compute all of them. The corners are numbered starting from the minimum position (m_Data[0]), going
	// counter-clockwise in the bottom plane, and then in the same order for the top plane (starting from the corner
	// that's directly above the minimum position). Hence, corner0 is pMin and corner6 is pMax, so we don't need to
	// custom-create those.
	
	CVector3D corner0; // corner0 is pMin, no need to copy it
	CVector3D corner1(pMax.X, pMin.Y, pMin.Z);
	CVector3D corner3(pMin.X, pMin.Y, pMax.Z);
	CVector3D corner4(pMin.X, pMax.Y, pMin.Z);
	CVector3D corner6; // corner6 is pMax, no need to copy it

	// transform corners to world space
	corner0 = transform.Transform(pMin); // = corner0
	corner1 = transform.Transform(corner1);
	corner3 = transform.Transform(corner3);
	corner4 = transform.Transform(corner4);
	corner6 = transform.Transform(pMax); // = corner6

	// Compute orientation vectors, half-size vector, and box center. We can get the orientation vectors by just taking
	// the directional vectors from a specific corner point (corner0) to the other corners, once in each direction. The
	// half-sizes are similarly computed by taking the distances of those sides and dividing them by 2. Finally, the 
	// center is simply the middle between the transformed pMin and pMax corners.

	const CVector3D sideU(corner1 - corner0);
	const CVector3D sideV(corner4 - corner0);
	const CVector3D sideW(corner3 - corner0);

	result.m_Basis[0] = sideU.Normalized();
	result.m_Basis[1] = sideV.Normalized();
	result.m_Basis[2] = sideW.Normalized();

	result.m_HalfSizes = CVector3D(
		sideU.Length()/2.f,
		sideV.Length()/2.f,
		sideW.Length()/2.f
	);

	result.m_Center = (corner0 + corner6) * 0.5f;
}


///////////////////////////////////////////////////////////////////////////////
// Intersect with the given frustum in a conservative manner
void CBoundingBoxAligned::IntersectFrustumConservative(const CFrustum& frustum)
{
	CBrush brush(*this);
	CBrush buf;

	brush.Intersect(frustum, buf);

	buf.Bounds(*this);
}


///////////////////////////////////////////////////////////////////////////////
void CBoundingBoxAligned::Expand(float amount)
{
	m_Data[0] -= CVector3D(amount, amount, amount);
	m_Data[1] += CVector3D(amount, amount, amount);
}

///////////////////////////////////////////////////////////////////////////////
// Render the bounding box
void CBoundingBoxAligned::Render(CShaderProgramPtr& shader) const
{
	std::vector<float> data;

#define ADD_FACE(x, y, z) \
	ADD_PT(0, 0, x, y, z); ADD_PT(1, 0, x, y, z); ADD_PT(1, 1, x, y, z); \
	ADD_PT(1, 1, x, y, z); ADD_PT(0, 1, x, y, z); ADD_PT(0, 0, x, y, z);
#define ADD_PT(u_, v_, x, y, z) \
	STMT(int u = u_; int v = v_; \
		data.push_back(u); \
		data.push_back(v); \
		data.push_back(m_Data[x].X); \
		data.push_back(m_Data[y].Y); \
		data.push_back(m_Data[z].Z); \
	)

	ADD_FACE(u, v, 0);
	ADD_FACE(0, u, v);
	ADD_FACE(u, 0, 1-v);
	ADD_FACE(u, 1-v, 1);
	ADD_FACE(1, u, 1-v);
	ADD_FACE(u, 1, v);

#undef ADD_FACE

	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 5*sizeof(float), &data[0]);
	shader->VertexPointer(3, GL_FLOAT, 5*sizeof(float), &data[2]);

	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6*6);
}

void CBoundingBoxAligned::RenderOutline(CShaderProgramPtr& shader) const
{
	std::vector<float> data;

#define ADD_FACE(x, y, z) \
	ADD_PT(0, 0, x, y, z); ADD_PT(1, 0, x, y, z); \
	ADD_PT(1, 0, x, y, z); ADD_PT(1, 1, x, y, z); \
	ADD_PT(1, 1, x, y, z); ADD_PT(0, 1, x, y, z); \
	ADD_PT(0, 1, x, y, z); ADD_PT(0, 0, x, y, z);
#define ADD_PT(u_, v_, x, y, z) \
	STMT(int u = u_; int v = v_; \
	data.push_back(u); \
	data.push_back(v); \
	data.push_back(m_Data[x].X); \
	data.push_back(m_Data[y].Y); \
	data.push_back(m_Data[z].Z); \
	)

	ADD_FACE(u, v, 0);
	ADD_FACE(0, u, v);
	ADD_FACE(u, 0, 1-v);
	ADD_FACE(u, 1-v, 1);
	ADD_FACE(1, u, 1-v);
	ADD_FACE(u, 1, v);

#undef ADD_FACE

	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 5*sizeof(float), &data[0]);
	shader->VertexPointer(3, GL_FLOAT, 5*sizeof(float), &data[2]);

	shader->AssertPointersBound();
	glDrawArrays(GL_LINES, 0, 6*8);
}
