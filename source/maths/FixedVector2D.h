/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_FIXED_VECTOR2D
#define INCLUDED_FIXED_VECTOR2D

#include "maths/Fixed.h"
#include "maths/Sqrt.h"

class CFixedVector2D
{
private:
	typedef CFixed_23_8 fixed;

public:
	fixed X, Y;

	CFixedVector2D() { }

	CFixedVector2D(fixed X, fixed Y) : X(X), Y(Y) { }

	/// Vector equality
	bool operator==(const CFixedVector2D& v) const
	{
		return (X == v.X && Y == v.Y);
	}

	/// Vector addition
	CFixedVector2D operator+(const CFixedVector2D& v) const
	{
		return CFixedVector2D(X + v.X, Y + v.Y);
	}

	/// Vector subtraction
	CFixedVector2D operator-(const CFixedVector2D& v) const
	{
		return CFixedVector2D(X - v.X, Y - v.Y);
	}

	/// Negation
	CFixedVector2D operator-() const
	{
		return CFixedVector2D(-X, -Y);
	}

	/// Vector addition
	CFixedVector2D& operator+=(const CFixedVector2D& v)
	{
		*this = *this + v;
		return *this;
	}

	/// Vector subtraction
	CFixedVector2D& operator-=(const CFixedVector2D& v)
	{
		*this = *this - v;
		return *this;
	}


	/**
	 * Returns the length of the vector.
	 * Will not overflow if the result can be represented as type 'fixed'.
	 */
	fixed Length() const
	{
		// Do intermediate calculations with 64-bit ints to avoid overflows
		i64 x = (i64)X.GetInternalValue();
		i64 y = (i64)Y.GetInternalValue();
		u64 d2 = (u64)(x * x + y * y);
		u32 d = isqrt64(d2);
		fixed r;
		r.SetInternalValue((i32)d);
		return r;
	}

	/**
	 * Normalize the vector so that length is close to 1.
	 * If length is 0, does nothing.
	 * WARNING: The fixed-point numbers only have 8-bit fractional parts, so
	 * a normalized vector will be very imprecise.
	 */
	void Normalize()
	{
		fixed l = Length();
		if (!l.IsZero())
		{
			X = X / l;
			Y = Y / l;
		}
	}

	/**
	 * Compute the dot product of this vector with another.
	 */
	fixed Dot(const CFixedVector2D& v)
	{
		i64 x = (i64)X.GetInternalValue() * (i64)v.X.GetInternalValue();
		i64 y = (i64)Y.GetInternalValue() * (i64)v.Y.GetInternalValue();
		i64 sum = x + y;
		fixed ret;
		ret.SetInternalValue((i32)(sum >> fixed::fract_bits));
		return ret;
	}
};

#endif // INCLUDED_FIXED_VECTOR2D
