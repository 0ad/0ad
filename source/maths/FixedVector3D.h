/* Copyright (C) 2013 Wildfire Games.
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
public:
	fixed X, Y, Z;

	CFixedVector3D() { }

	CFixedVector3D(fixed X, fixed Y, fixed Z) : X(X), Y(Y), Z(Z) { }

	/// Vector equality
	bool operator==(const CFixedVector3D& v) const
	{
		return (X == v.X && Y == v.Y && Z == v.Z);
	}

	/// Vector inequality
	bool operator!=(const CFixedVector3D& v) const
	{
		return (X != v.X || Y != v.Y || Z != v.Z);
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
		i32 x = X.GetInternalValue();
		i32 y = Y.GetInternalValue();
		i32 z = Z.GetInternalValue();
		u64 xx = (u64)FIXED_MUL_I64_I32_I32(x, x);
		u64 yy = (u64)FIXED_MUL_I64_I32_I32(y, y);
		u64 zz = (u64)FIXED_MUL_I64_I32_I32(z, z);
		u64 t = xx + yy;
		CheckUnsignedAdditionOverflow(t, xx, L"Overflow in CFixedVector3D::Length() part 1")

		u64 d2 = t + zz;
		CheckUnsignedAdditionOverflow(d2, t, L"Overflow in CFixedVector3D::Length() part 2")

		u32 d = isqrt64(d2);

		CheckU32CastOverflow(d, i32, L"Overflow in CFixedVector3D::Length() part 3")
		fixed r;
		r.SetInternalValue((i32)d);
		return r;
	}

	/**
	 * Normalize the vector so that length is close to 1.
	 * If length is 0, does nothing.
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
		fixed l = Length();
		if (!l.IsZero())
		{
			X = X.MulDiv(n, l);
			Y = Y.MulDiv(n, l);
			Z = Z.MulDiv(n, l);
		}
	}

	/**
	 * Compute the cross product of this vector with another.
	 */
	CFixedVector3D Cross(const CFixedVector3D& v)
	{
		i64 y_vz = FIXED_MUL_I64_I32_I32(Y.GetInternalValue(), v.Z.GetInternalValue());
		i64 z_vy = FIXED_MUL_I64_I32_I32(Z.GetInternalValue(), v.Y.GetInternalValue());
		CheckSignedSubtractionOverflow(i64, y_vz, z_vy, L"Overflow in CFixedVector3D::Cross() part 1", L"Underflow in CFixedVector3D::Cross() part 1")
		i64 x = y_vz - z_vy;
		x >>= fixed::fract_bits;

		i64 z_vx = FIXED_MUL_I64_I32_I32(Z.GetInternalValue(), v.X.GetInternalValue());
		i64 x_vz = FIXED_MUL_I64_I32_I32(X.GetInternalValue(), v.Z.GetInternalValue());
		CheckSignedSubtractionOverflow(i64, z_vx, x_vz, L"Overflow in CFixedVector3D::Cross() part 2", L"Underflow in CFixedVector3D::Cross() part 2")
		i64 y = z_vx - x_vz;
		y >>= fixed::fract_bits;

		i64 x_vy = FIXED_MUL_I64_I32_I32(X.GetInternalValue(), v.Y.GetInternalValue());
		i64 y_vx = FIXED_MUL_I64_I32_I32(Y.GetInternalValue(), v.X.GetInternalValue());
		CheckSignedSubtractionOverflow(i64, x_vy, y_vx, L"Overflow in CFixedVector3D::Cross() part 3", L"Underflow in CFixedVector3D::Cross() part 3")
		i64 z = x_vy - y_vx;
		z >>= fixed::fract_bits;

		CheckCastOverflow(x, i32, L"Overflow in CFixedVector3D::Cross() part 4", L"Underflow in CFixedVector3D::Cross() part 4")
		CheckCastOverflow(y, i32, L"Overflow in CFixedVector3D::Cross() part 5", L"Underflow in CFixedVector3D::Cross() part 5")
		CheckCastOverflow(z, i32, L"Overflow in CFixedVector3D::Cross() part 6", L"Underflow in CFixedVector3D::Cross() part 6")
		CFixedVector3D ret;
		ret.X.SetInternalValue((i32)x);
		ret.Y.SetInternalValue((i32)y);
		ret.Z.SetInternalValue((i32)z);
		return ret;
	}

	/**
	 * Compute the dot product of this vector with another.
	 */
	fixed Dot(const CFixedVector3D& v)
	{
		i64 x = FIXED_MUL_I64_I32_I32(X.GetInternalValue(), v.X.GetInternalValue());
		i64 y = FIXED_MUL_I64_I32_I32(Y.GetInternalValue(), v.Y.GetInternalValue());
		i64 z = FIXED_MUL_I64_I32_I32(Z.GetInternalValue(), v.Z.GetInternalValue());
		CheckSignedAdditionOverflow(i64, x, y, L"Overflow in CFixedVector3D::Dot() part 1", L"Underflow in CFixedVector3D::Dot() part 1")
		i64 t = x + y;

		CheckSignedAdditionOverflow(i64, t, z, L"Overflow in CFixedVector3D::Dot() part 2", L"Underflow in CFixedVector3D::Dot() part 2")
		i64 sum = t + z;
		sum >>= fixed::fract_bits;
		CheckCastOverflow(sum, i32, L"Overflow in CFixedVector3D::Dot() part 3", L"Underflow in CFixedVector3D::Dot() part 3")

		fixed ret;
		ret.SetInternalValue((i32)sum);
		return ret;
	}
};

#endif // INCLUDED_FIXED_VECTOR3D
