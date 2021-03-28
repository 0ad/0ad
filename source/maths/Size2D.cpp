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

#include "Size2D.h"

CSize2D::CSize2D() = default;

CSize2D::CSize2D(const CSize2D& size) : Width(size.Width), Height(size.Height)
{
}

CSize2D::CSize2D(const float width, const float height) : Width(width), Height(height)
{
}

CSize2D& CSize2D::operator=(const CSize2D& size)
{
	Width = size.Width;
	Height = size.Height;
	return *this;
}

bool CSize2D::operator==(const CSize2D& size) const
{
	return Width == size.Width && Height == size.Height;
}

bool CSize2D::operator!=(const CSize2D& size) const
{
	return !(*this == size);
}

CSize2D CSize2D::operator+(const CSize2D& size) const
{
	return CSize2D(Width + size.Width, Height + size.Height);
}

CSize2D CSize2D::operator-(const CSize2D& size) const
{
	return CSize2D(Width - size.Width, Height - size.Height);
}

CSize2D CSize2D::operator/(const float a) const
{
	return CSize2D(Width / a, Height / a);
}

CSize2D CSize2D::operator*(const float a) const
{
	return CSize2D(Width * a, Height * a);
}

void CSize2D::operator+=(const CSize2D& size)
{
	Width += size.Width;
	Height += size.Height;
}

void CSize2D::operator-=(const CSize2D& size)
{
	Width -= size.Width;
	Height -= size.Height;
}

void CSize2D::operator/=(const float a)
{
	Width /= a;
	Height /= a;
}

void CSize2D::operator*=(const float a)
{
	Width *= a;
	Height *= a;
}
