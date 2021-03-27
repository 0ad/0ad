/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_SHAPES
#define INCLUDED_SHAPES

class CPos;
class CSize2D;


/**
 * Rectangle class used for screen rectangles. It's very similar to the MS
 * CRect, but with FLOATS because it's meant to be used with OpenGL which
 * takes float values.
 */
class CRect
{
public:
	CRect();
	CRect(const CPos &pos);
	CRect(const CSize2D &size);
	CRect(const CPos &upperleft, const CPos &bottomright);
	CRect(const CPos &pos, const CSize2D &size);
	CRect(const float l, const float t, const float r, const float b);
	CRect(const CRect&);

	CRect& operator=(const CRect& a);
	bool operator==(const CRect& a) const;
	bool operator!=(const CRect& a) const;
	CRect operator-() const;
	CRect operator+() const;

	CRect operator+(const CRect& a) const;
	CRect operator+(const CPos& a) const;
	CRect operator+(const CSize2D& a) const;
	CRect operator-(const CRect& a) const;
	CRect operator-(const CPos& a) const;
	CRect operator-(const CSize2D& a) const;

	void operator+=(const CRect& a);
	void operator+=(const CPos& a);
	void operator+=(const CSize2D& a);
	void operator-=(const CRect& a);
	void operator-=(const CPos& a);
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
	CPos TopLeft() const;

	/**
	 * Get Position equivalent to top/right corner
	 */
	CPos TopRight() const;

	/**
	 * Get Position equivalent to bottom/left corner
	 */
	CPos BottomLeft() const;

	/**
	 * Get Position equivalent to bottom/right corner
	 */
	CPos BottomRight() const;

	/**
	 * Get Position equivalent to the center of the rectangle
	 */
	CPos CenterPoint() const;

	/**
	 * Evalutates if point is within the rectangle
	 * @param point CPos representing point
	 * @return true if inside.
	 */
	bool PointInside(const CPos &point) const;

	CRect Scale(float x, float y) const;

	/**
	 * Returning CPos representing each corner.
	 */

public:
	/**
	 * Dimensions
	 */
	float left, top, right, bottom;
};

/**
 * Made to represent screen positions and delta values.
 * @see CRect
 * @see CSize2D
 */
class CPos
{
public:
	CPos();
	CPos(const CPos& pos);
	CPos(const CSize2D &pos);
	CPos(const float px, const float py);

	CPos& operator=(const CPos& a);
	bool operator==(const CPos& a) const;
	bool operator!=(const CPos& a) const;
	CPos operator-() const;
	CPos operator+() const;

	CPos operator+(const CPos& a) const;
	CPos operator+(const CSize2D& a) const;
	CPos operator-(const CPos& a) const;
	CPos operator-(const CSize2D& a) const;

	void operator+=(const CPos& a);
	void operator+=(const CSize2D& a);
	void operator-=(const CPos& a);
	void operator-=(const CSize2D& a);

public:
	/**
	 * Position
	 */
	float x, y;
};

#endif // INCLUDED_SHAPES
