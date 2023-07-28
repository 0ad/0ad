/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_VECTOR2D
#define INCLUDED_VECTOR2D

#include "ps/containers/Span.h"

#include <math.h>

class CSize2D;

/*
 * Provides an interface for a vector in R2 and allows vector and
 * scalar operations on it.
 */
class CVector2D
{
public:
	CVector2D() : X(0.0f), Y(0.0f) {}
	CVector2D(float x, float y) : X(x), Y(y) {}
	CVector2D(const CSize2D& size);

	operator float*()
	{
		return &X;
	}

	operator const float*() const
	{
		return &X;
	}

	bool operator==(const CVector2D& v) const;
	bool operator!=(const CVector2D& v) const;

	CVector2D operator-() const
	{
		return CVector2D(-X, -Y);
	}

	CVector2D operator+(const CVector2D& t) const
	{
		return CVector2D(X + t.X, Y + t.Y);
	}

	CVector2D operator-(const CVector2D& t) const
	{
		return CVector2D(X - t.X, Y - t.Y);
	}

	CVector2D operator*(float f) const
	{
		return CVector2D(X * f, Y * f);
	}

	CVector2D operator/(float f) const
	{
		float inv = 1.0f / f;
		return CVector2D(X * inv, Y * inv);
	}

	CVector2D& operator+=(const CVector2D& t)
	{
		X += t.X;
		Y += t.Y;
		return *this;
	}

	CVector2D& operator-=(const CVector2D& t)
	{
		X -= t.X;
		Y -= t.Y;
		return *this;
	}

	CVector2D& operator*=(float f)
	{
		X *= f;
		Y *= f;
		return *this;
	}

	CVector2D& operator/=(float f)
	{
		float invf = 1.0f / f;
		X *= invf;
		Y *= invf;
		return *this;
	}

	float Dot(const CVector2D& a) const
	{
		return X * a.X + Y * a.Y;
	}

	float LengthSquared() const
	{
		return Dot(*this);
	}

	float Length() const
	{
		return (float)sqrt(LengthSquared());
	}

	void Normalize()
	{
		float mag = Length();
		X /= mag;
		Y /= mag;
	}

	CVector2D Normalized() const
	{
		float mag = Length();
		return CVector2D(X / mag, Y / mag);
	}

	/**
	 * Returns a version of this vector rotated counterclockwise by @p angle radians.
	 */
	CVector2D Rotated(float angle) const
	{
		float c = cosf(angle);
		float s = sinf(angle);
		return CVector2D(
			c*X - s*Y,
			s*X + c*Y
		);
	}

	/**
	 * Rotates this vector counterclockwise by @p angle radians.
	 */
	void Rotate(float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);
		float newX = c*X - s*Y;
		float newY = s*X + c*Y;
		X = newX;
		Y = newY;
	}

	CVector2D operator+(const CSize2D& size) const;
	CVector2D operator-(const CSize2D& size) const;

	void operator+=(const CSize2D& size);
	void operator-=(const CSize2D& size);

	// Returns 2 element array of floats, e.g. for vec2 uniforms.
	PS::span<const float> AsFloatArray() const
	{
		// Additional check to prevent a weird compiler having a different
		// alignement for an array and a class members.
		static_assert(
			sizeof(CVector2D) == sizeof(float) * 2u &&
			offsetof(CVector2D, X) == 0 &&
			offsetof(CVector2D, Y) == sizeof(float),
			"Vector2D should be properly layouted to use AsFloatArray");
		return PS::span<const float>(&X, 2);
	}

public:
	float X, Y;
};

#endif // INCLUDED_VECTOR2D
