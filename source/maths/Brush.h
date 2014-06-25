/* Copyright (C) 2012 Wildfire Games.
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
 * CBrush, a class representing a convex object
 */

#ifndef maths_brush_h
#define maths_brush_h

#include "Vector3D.h"

#include "graphics/ShaderProgram.h"

class CBoundingBoxAligned;
class CFrustum;
class CPlane;


/**
 * Class CBrush: Represents a convex object, supports some CSG operations.
 */
class CBrush
{
	friend class TestBrush;

public:
	CBrush() { }

	/**
	 * CBrush: Construct a brush from a bounds object.
	 *
	 * @param bounds the CBoundingBoxAligned object to construct the brush from.
	 */
	CBrush(const CBoundingBoxAligned& bounds);

	/**
	 * IsEmpty: Returns whether the brush is empty.
	 *
	 * @return @c true if the brush is empty, @c false otherwise
	 */
	bool IsEmpty() const { return m_Vertices.size() == 0; }

	/**
	 * Bounds: Calculate the axis-aligned bounding box for this brush.
	 *
	 * @param result the resulting bounding box is stored here
	 */
	void Bounds(CBoundingBoxAligned& result) const;

	/**
	 * Slice: Cut the object along the given plane, resulting in a smaller (or even empty) brush representing 
	 * the part of the object that lies in front of the plane (as defined by the positive direction of its 
	 * normal vector).
	 *
	 * @param plane the slicing plane
	 * @param result the resulting brush is stored here
	 */
	void Slice(const CPlane& plane, CBrush& result) const;

	/**
	 * Intersect: Intersect the brush with the given frustum.
	 *
	 * @param frustum the frustum to intersect with
	 * @param result the resulting brush is stored here
	 */
	void Intersect(const CFrustum& frustum, CBrush& result) const;

	/**
	 * Render the surfaces of the brush as triangles.
	 */
	void Render(CShaderProgramPtr& shader) const;

	/**
	 * Render the outline of the brush as lines.
	 */
	void RenderOutline(CShaderProgramPtr& shader) const;

private:
	
	/**
	 * Returns a copy of the vertices in this brush. Intended for testing purposes; you should not need to use
	 * this method directly.
	 */
	std::vector<CVector3D> GetVertices() const;

	/**
	 * Writes a vector of the faces in this brush to @p out. Each face is itself a vector, listing the vertex indices 
	 * that make up the face, starting and ending with the same index. Intended for testing purposes; you should not 
	 * need to use this method directly.
	 */
	void GetFaces(std::vector<std::vector<size_t> >& out) const;

private:
	static const size_t NO_VERTEX = ~0u;

	typedef std::vector<CVector3D> Vertices;
	typedef std::vector<size_t> FaceIndices;

	/// Collection of unique vertices that make up this shape.
	Vertices m_Vertices;

	/**
	 * Holds the face definitions of this brush. Each face is a sequence of indices into m_Vertices that starts and ends with 
	 * the same vertex index, completing a loop through all the vertices that make up the face. This vector holds all the face
	 * sequences back-to-back, thus looking something like 'x---xy--------yz--z' in the general case.
	 */
	FaceIndices m_Faces;

	struct Helper;
};

#endif // maths_brush_h
