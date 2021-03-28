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

#include "precompiled.h"

#include "Shapes.h"

#include "maths/Size2D.h"

CRect::CRect() :
	left(0.f), top(0.f), right(0.f), bottom(0.f)
{
}

CRect::CRect(const CRect& rect) :
	left(rect.left), top(rect.top), right(rect.right), bottom(rect.bottom)
{
}

CRect::CRect(const CPos &pos) :
	left(pos.x), top(pos.y), right(pos.x), bottom(pos.y)
{
}

CRect::CRect(const CSize2D& size) :
	left(0.f), top(0.f), right(size.Width), bottom(size.Height)
{
}

CRect::CRect(const CPos& upperleft, const CPos& bottomright) :
	left(upperleft.x), top(upperleft.y), right(bottomright.x), bottom(bottomright.y)
{
}

CRect::CRect(const CPos& pos, const CSize2D& size) :
	left(pos.x), top(pos.y), right(pos.x + size.Width), bottom(pos.y + size.Height)
{
}

CRect::CRect(const float l, const float t, const float r, const float b) :
	left(l), top(t), right(r), bottom(b)
{
}

CRect& CRect::operator=(const CRect& a)
{
	left = a.left;
	top = a.top;
	right = a.right;
	bottom = a.bottom;
	return *this;
}

bool CRect::operator==(const CRect &a) const
{
	return	(left == a.left &&
			 top == a.top &&
			 right == a.right &&
			 bottom == a.bottom);
}

bool CRect::operator!=(const CRect& a) const
{
	return !(*this == a);
}

CRect CRect::operator-() const
{
	return CRect(-left, -top, -right, -bottom);
}

CRect CRect::operator+() const
{
	return *this;
}

CRect CRect::operator+(const CRect& a) const
{
	return CRect(left + a.left, top + a.top, right + a.right, bottom + a.bottom);
}

CRect CRect::operator+(const CPos& a) const
{
	return CRect(left + a.x, top + a.y, right + a.x, bottom + a.y);
}

CRect CRect::operator+(const CSize2D& a) const
{
	return CRect(left + a.Width, top + a.Height, right + a.Width, bottom + a.Height);
}

CRect CRect::operator-(const CRect& a) const
{
	return CRect(left - a.left, top - a.top, right - a.right, bottom - a.bottom);
}

CRect CRect::operator-(const CPos& a) const
{
	return CRect(left - a.x, top - a.y, right - a.x, bottom - a.y);
}

CRect CRect::operator-(const CSize2D& a) const
{
	return CRect(left - a.Width, top - a.Height, right - a.Width, bottom - a.Height);
}

void CRect::operator+=(const CRect& a)
{
	left += a.left;
	top += a.top;
	right += a.right;
	bottom += a.bottom;
}

void CRect::operator+=(const CPos& a)
{
	left += a.x;
	top += a.y;
	right += a.x;
	bottom += a.y;
}

void CRect::operator+=(const CSize2D& a)
{
	left += a.Width;
	top += a.Height;
	right += a.Width;
	bottom += a.Height;
}

void CRect::operator-=(const CRect& a)
{
	left -= a.left;
	top -= a.top;
	right -= a.right;
	bottom -= a.bottom;
}

void CRect::operator-=(const CPos& a)
{
	left -= a.x;
	top -= a.y;
	right -= a.x;
	bottom -= a.y;
}

void CRect::operator-=(const CSize2D& a)
{
	left -= a.Width;
	top -= a.Height;
	right -= a.Width;
	bottom -= a.Height;
}

float CRect::GetWidth() const
{
	return right-left;
}

float CRect::GetHeight() const
{
	return bottom-top;
}

CSize2D CRect::GetSize() const
{
	return CSize2D(right - left, bottom - top);
}

CPos CRect::TopLeft() const
{
	return CPos(left, top);
}

CPos CRect::TopRight() const
{
	return CPos(right, top);
}

CPos CRect::BottomLeft() const
{
	return CPos(left, bottom);
}

CPos CRect::BottomRight() const
{
	return CPos(right, bottom);
}

CPos CRect::CenterPoint() const
{
	return CPos((left + right) / 2.f, (top + bottom) / 2.f);
}

bool CRect::PointInside(const CPos &point) const
{
	return (point.x >= left &&
			point.x <= right &&
			point.y >= top &&
			point.y <= bottom);
}

CRect CRect::Scale(float x, float y) const
{
	return CRect(left * x, top * y, right * x, bottom * y);
}

/*************************************************************************/

CPos::CPos() : x(0.f), y(0.f)
{
}

CPos::CPos(const CPos& pos) : x(pos.x), y(pos.y)
{
}

CPos::CPos(const CSize2D& s) : x(s.Width), y(s.Height)
{
}

CPos::CPos(const float px, const float py) : x(px), y(py)
{
}

CPos& CPos::operator=(const CPos& a)
{
	x = a.x;
	y = a.y;
	return *this;
}

bool CPos::operator==(const CPos &a) const
{
	return x == a.x && y == a.y;
}

bool CPos::operator!=(const CPos& a) const
{
	return !(*this == a);
}

CPos CPos::operator-() const
{
	return CPos(-x, -y);
}

CPos CPos::operator+() const
{
	return *this;
}

CPos CPos::operator+(const CPos& a) const
{
	return CPos(x + a.x, y + a.y);
}

CPos CPos::operator+(const CSize2D& a) const
{
	return CPos(x + a.Width, y + a.Height);
}

CPos CPos::operator-(const CPos& a) const
{
	return CPos(x - a.x, y - a.y);
}

CPos CPos::operator-(const CSize2D& a) const
{
	return CPos(x - a.Width, y - a.Height);
}

void CPos::operator+=(const CPos& a)
{
	x += a.x;
	y += a.y;
}

void CPos::operator+=(const CSize2D& a)
{
	x += a.Width;
	y += a.Height;
}

void CPos::operator-=(const CPos& a)
{
	x -= a.x;
	y -= a.y;
}

void CPos::operator-=(const CSize2D& a)
{
	x -= a.Width;
	y -= a.Height;
}
