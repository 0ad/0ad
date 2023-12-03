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

#include "Rect.h"

#include "maths/Size2D.h"
#include "maths/Vector2D.h"

CRect::CRect() :
	left(0.f), top(0.f), right(0.f), bottom(0.f)
{
}

CRect::CRect(const CRect& rect) :
	left(rect.left), top(rect.top), right(rect.right), bottom(rect.bottom)
{
}

CRect::CRect(const CVector2D& pos) :
	left(pos.X), top(pos.Y), right(pos.X), bottom(pos.Y)
{
}

CRect::CRect(const CSize2D& size) :
	left(0.f), top(0.f), right(size.Width), bottom(size.Height)
{
}

CRect::CRect(const CVector2D& upperleft, const CVector2D& bottomright) :
	left(upperleft.X), top(upperleft.Y), right(bottomright.X), bottom(bottomright.Y)
{
}

CRect::CRect(const CVector2D& pos, const CSize2D& size) :
	left(pos.X), top(pos.Y), right(pos.X + size.Width), bottom(pos.Y + size.Height)
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

CRect CRect::operator+(const CVector2D& a) const
{
	return CRect(left + a.X, top + a.Y, right + a.X, bottom + a.Y);
}

CRect CRect::operator+(const CSize2D& a) const
{
	return CRect(left + a.Width, top + a.Height, right + a.Width, bottom + a.Height);
}

CRect CRect::operator-(const CRect& a) const
{
	return CRect(left - a.left, top - a.top, right - a.right, bottom - a.bottom);
}

CRect CRect::operator-(const CVector2D& a) const
{
	return CRect(left - a.X, top - a.Y, right - a.X, bottom - a.Y);
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

void CRect::operator+=(const CVector2D& a)
{
	left += a.X;
	top += a.Y;
	right += a.X;
	bottom += a.Y;
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

void CRect::operator-=(const CVector2D& a)
{
	left -= a.X;
	top -= a.Y;
	right -= a.X;
	bottom -= a.Y;
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

CVector2D CRect::TopLeft() const
{
	return CVector2D(left, top);
}

CVector2D CRect::TopRight() const
{
	return CVector2D(right, top);
}

CVector2D CRect::BottomLeft() const
{
	return CVector2D(left, bottom);
}

CVector2D CRect::BottomRight() const
{
	return CVector2D(right, bottom);
}

CVector2D CRect::CenterPoint() const
{
	return CVector2D((left + right) / 2.f, (top + bottom) / 2.f);
}

bool CRect::PointInside(const CVector2D& point) const
{
	return (point.X >= left &&
			point.X <= right &&
			point.Y >= top &&
			point.Y <= bottom);
}

CRect CRect::Scale(float x, float y) const
{
	return CRect(left * x, top * y, right * x, bottom * y);
}
