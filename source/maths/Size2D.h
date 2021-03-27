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

#ifndef INCLUDED_SIZE2D
#define INCLUDED_SIZE2D

/*
 * Provides an interface for a size - geometric property in R2.
 */
class CSize2D
{
public:
	CSize2D();
	CSize2D(const CSize2D& size);
	CSize2D(const float width, const float height);

	CSize2D& operator=(const CSize2D& size);
	bool operator==(const CSize2D& size) const;
	bool operator!=(const CSize2D& size) const;

	CSize2D operator+(const CSize2D& size) const;
	CSize2D operator-(const CSize2D& size) const;
	CSize2D operator/(const float a) const;
	CSize2D operator*(const float a) const;

	void operator+=(const CSize2D& a);
	void operator-=(const CSize2D& a);
	void operator/=(const float a);
	void operator*=(const float a);

public:
	float Width = 0.0f, Height = 0.0f;
};

#endif // INCLUDED_SIZE2D
