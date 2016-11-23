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
 * Provides an interface for a vector in R4 and allows vector and
 * scalar operations on it
 */

#ifndef INCLUDED_VECTOR4D
#define INCLUDED_VECTOR4D

#include <math.h>

class CVector4D
{
public:
	CVector4D() : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) { }

	CVector4D(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) { }

	bool operator==(const CVector4D& t) const
	{
		return (X == t.X && Y == t.Y && Z == t.Z && W == t.W);
	}

	bool operator!=(const CVector4D& t) const
	{
		return !(*this == t);
	}

	CVector4D operator-() const
	{
		return CVector4D(-X, -Y, -Z, -W);
	}

	CVector4D operator+(const CVector4D& t) const
	{
		return CVector4D(X+t.X, Y+t.Y, Z+t.Z, W+t.W);
	}

	CVector4D operator-(const CVector4D& t) const
	{
		return CVector4D(X-t.X, Y-t.Y, Z-t.Z, W-t.W);
	}

	CVector4D operator*(const CVector4D& t) const
	{
		return CVector4D(X*t.X, Y*t.Y, Z*t.Z, W*t.W);
	}

	CVector4D operator*(float f) const
	{
		return CVector4D(X*f, Y*f, Z*f, W*f);
	}

	CVector4D operator/(float f) const
	{
		float inv_f = 1.0f / f;
		return CVector4D(X*inv_f, Y*inv_f, Z*inv_f, W*inv_f);
	}

	CVector4D& operator+=(const CVector4D& t)
	{
		X += t.X;
		Y += t.Y;
		Z += t.Z;
		W += t.W;
		return *this;
	}

	CVector4D& operator-=(const CVector4D& t)
	{
		X -= t.X;
		Y -= t.Y;
		Z -= t.Z;
		W -= t.W;
		return *this;
	}

	CVector4D& operator*=(const CVector4D& t)
	{
		X *= t.X;
		Y *= t.Y;
		Z *= t.Z;
		W *= t.W;
		return *this;
	}

	CVector4D& operator*=(float f)
	{
		X *= f;
		Y *= f;
		Z *= f;
		W *= f;
		return *this;
	}

	CVector4D& operator/=(float f)
	{
		float inv_f = 1.0f / f;
		X *= inv_f;
		Y *= inv_f;
		Z *= inv_f;
		W *= inv_f;
		return *this;
	}

	float Dot(const CVector4D& a) const
	{
		return X*a.X + Y*a.Y + Z*a.Z + W*a.W;
	}


	float X, Y, Z, W;
};

#endif
