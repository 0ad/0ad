/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_RECT
#define INCLUDED_RECT

class CSize2D;
class CVector2D;


/**
 * Rectangle class used for screen rectangles. It's very similar to the MS
 * CRect, but with FLOATS because it's meant to be used with OpenGL which
 * takes float values.
 */
class CRect
{
public:
	CRect();
	CRect(const CVector2D& pos);
	CRect(const CSize2D& size);
	CRect(const CVector2D& upperleft, const CVector2D& bottomright);
	CRect(const CVector2D& pos, const CSize2D& size);
	CRect(const float l, const float t, const float r, const float b);
	CRect(const CRect&);

	CRect& operator=(const CRect& a);
	bool operator==(const CRect& a) const;
	bool operator!=(const CRect& a) const;
	CRect operator-() const;
	CRect operator+() const;

	CRect operator+(const CRect& a) const;
	CRect operator+(const CVector2D& a) const;
	CRect operator+(const CSize2D& a) const;
	CRect operator-(const CRect& a) const;
	CRect operator-(const CVector2D& a) const;
	CRect operator-(const CSize2D& a) const;

	void operator+=(const CRect& a);
	void operator+=(const CVector2D& a);
	void operator+=(const CSize2D& a);
	void operator-=(const CRect& a);
	void operator-=(const CVector2D& a);
	void operator-=(const CSize2D& a);

	/**
	 * @return Width of Rectangle
	 */
	float GetWidth() const;

	/**
	 * @return Height of Rectangle
	 */
	float GetHeight() const;

	/**
	 * Get Size
	 */
	CSize2D GetSize() const;

	/**
	 * Get Position equivalent to top/left corner
	 */
	CVector2D TopLeft() const;

	/**
	 * Get Position equivalent to top/right corner
	 */
	CVector2D TopRight() const;

	/**
	 * Get Position equivalent to bottom/left corner
	 */
	CVector2D BottomLeft() const;

	/**
	 * Get Position equivalent to bottom/right corner
	 */
	CVector2D BottomRight() const;

	/**
	 * Get Position equivalent to the center of the rectangle
	 */
	CVector2D CenterPoint() const;

	/**
	 * Evalutates if point is within the rectangle
	 * @param point CVector2D representing point
	 * @return true if inside.
	 */
	bool PointInside(const CVector2D &point) const;

	CRect Scale(float x, float y) const;

	/**
	 * Returning CVector2D representing each corner.
	 */

public:
	/**
	 * Dimensions
	 */
	float left, top, right, bottom;
};

#endif // INCLUDED_RECT
