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

#ifndef INCLUDED_FIXED_VECTOR3D
#define INCLUDED_FIXED_VECTOR3D

#include "maths/Fixed.h"
#include "maths/Sqrt.h"

class CFixedVector3D
{
private:
	typedef CFixed_23_8 fixed;

public:
	fixed X, Y, Z;

	CFixedVector3D() { }

	CFixedVector3D(fixed X, fixed Y, fixed Z) : X(X), Y(Y), Z(Z) { }

	/// Vector equality
	bool operator==(const CFixedVector3D& v) const
	{
		return (X == v.X && Y == v.Y && Z == v.Z);
	}

	/// Vector addition
	CFixedVector3D operator+(const CFixedVector3D& v) const
	{
		return CFixedVector3D(X + v.X, Y + v.Y, Z + v.Z);
	}

	/// Vector subtraction
	CFixedVector3D operator-(const CFixedVector3D& v) const
	{
		return CFixedVector3D(X - v.X, Y - v.Y, Z - v.Z);
	}

	/// Negation
	CFixedVector3D operator-() const
	{
		return CFixedVector3D(-X, -Y, -Z);
	}

	/// Vector addition
	CFixedVector3D& operator+=(const CFixedVector3D& v)
	{
		*this = *this + v;
		return *this;
	}

	/// Vector subtraction
	CFixedVector3D& operator-=(const CFixedVector3D& v)
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
		i64 z = (i64)Z.GetInternalValue();
		u64 d2 = (u64)(x * x + y * y + z * z);
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
			Z = Z / l;
		}
	}

	/**
	 * Normalize the vector so that length is close to n.
	 * If length is 0, does nothing.
	 */
	void Normalize(fixed n)
	{
		if (n.IsZero())
		{
			X = Y = Z = fixed::FromInt(0);
			return;
		}

		fixed l = Length();
		// TODO: work out whether this is giving decent precision
		fixed d = l / n;
		if (!d.IsZero())
		{
			X = X / d;
			Y = Y / d;
			Z = Z / d;
		}
	}

	/**
	 * Compute the cross product of this vector with another.
	 */
	CFixedVector3D Cross(const CFixedVector3D& v)
	{
		i64 x = ((i64)Y.GetInternalValue() * (i64)v.Z.GetInternalValue()) - ((i64)Z.GetInternalValue() * (i64)v.Y.GetInternalValue());
		i64 y = ((i64)Z.GetInternalValue() * (i64)v.X.GetInternalValue()) - ((i64)X.GetInternalValue() * (i64)v.Z.GetInternalValue());
		i64 z = ((i64)X.GetInternalValue() * (i64)v.Y.GetInternalValue()) - ((i64)Y.GetInternalValue() * (i64)v.X.GetInternalValue());
		CFixedVector3D ret;
		ret.X.SetInternalValue((i32)(x >> fixed::fract_bits));
		ret.Y.SetInternalValue((i32)(y >> fixed::fract_bits));
		ret.Z.SetInternalValue((i32)(z >> fixed::fract_bits));
		return ret;
	}

	/**
	 * Compute the dot product of this vector with another.
	 */
	fixed Dot(const CFixedVector3D& v)
	{
		i64 x = (i64)X.GetInternalValue() * (i64)v.X.GetInternalValue();
		i64 y = (i64)Y.GetInternalValue() * (i64)v.Y.GetInternalValue();
		i64 z = (i64)Z.GetInternalValue() * (i64)v.Z.GetInternalValue();
		i64 sum = x + y + z;
		fixed ret;
		ret.SetInternalValue((i32)(sum >> fixed::fract_bits));
		return ret;
	}
};

#endif // INCLUDED_FIXED_VECTOR3D
