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

#include "Vector2D.h"

#include "maths/Size2D.h"

CVector2D::CVector2D(const CSize2D& size) : X(size.Width), Y(size.Height)
{
}

bool CVector2D::operator==(const CVector2D& v) const
{
	return X == v.X && Y == v.Y;
}

bool CVector2D::operator!=(const CVector2D& v) const
{
	return !(*this == v);
}

CVector2D CVector2D::operator+(const CSize2D& size) const
{
	return CVector2D(X + size.Width, Y + size.Height);
}

CVector2D CVector2D::operator-(const CSize2D& size) const
{
	return CVector2D(X - size.Width, Y - size.Height);
}

void CVector2D::operator+=(const CSize2D& size)
{
	X += size.Width;
	Y += size.Height;
}

void CVector2D::operator-=(const CSize2D& size)
{
	X -= size.Width;
	Y -= size.Height;
}
