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
 * CBrush, a class representing a convex object
 */

#ifndef maths_brush_h
#define maths_brush_h

#include "Vector3D.h"

class CBound;
class CFrustum;
class CPlane;


/**
 * Class CBrush: Represents a convex object, supports some CSG operations.
 */
class CBrush
{
public:
	CBrush() { }

	/**
	 * CBrush: Construct a brush from a bounds object.
	 *
	 * @param bounds the CBound object to construct the brush from.
	 */
	CBrush(const CBound& bounds);

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
	void Bounds(CBound& result) const;

	/**
	 * Slice: Cut the object along the given plane, resulting in a smaller (or even empty)
	 * brush representing the part of the object that lies in front of the plane.
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
	 * Render: Renders the brush as OpenGL polygons.
	 *
	 * @note the winding of the brush faces is undefined (i.e. it is undefined which
	 * sides of the faces are the front faces)
	 */
	void Render() const;

private:
	static const size_t no_vertex = ~0u;

	typedef std::vector<CVector3D> Vertices;
	typedef std::vector<size_t> FaceIndices;

	Vertices m_Vertices;
	FaceIndices m_Faces;

	struct Helper;
};

#endif // maths_brush_h
