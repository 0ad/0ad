/* Copyright (C) 2017 Wildfire Games.
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
public:
	fixed X, Y;

	CFixedVector2D() { }
	CFixedVector2D(fixed X, fixed Y) : X(X), Y(Y) { }

	/// Vector equality
	bool operator==(const CFixedVector2D& v) const
	{
		return (X == v.X && Y == v.Y);
	}

	/// Vector inequality
	bool operator!=(const CFixedVector2D& v) const
	{
		return (X != v.X || Y != v.Y);
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

	/// Scalar multiplication by an integer
	CFixedVector2D operator*(int n) const
	{
		return CFixedVector2D(X*n, Y*n);
	}

	/// Scalar division by an integer. Must not have n == 0.
	CFixedVector2D operator/(int n) const
	{
		return CFixedVector2D(X/n, Y/n);
	}

	/**
	 * Multiply by a CFixed. Likely to overflow if both numbers are large,
	 * so we use an ugly name instead of operator* to make it obvious.
	 */
	CFixedVector2D Multiply(fixed n) const
	{
		return CFixedVector2D(X.Multiply(n), Y.Multiply(n));
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
		u64 xx = (u64)FIXED_MUL_I64_I32_I32(x, x);
		u64 yy = (u64)FIXED_MUL_I64_I32_I32(y, y);
		u64 d2 = xx + yy;
		CheckUnsignedAdditionOverflow(d2, xx, L"Overflow in CFixedVector2D::Length() part 1")

		u32 d = isqrt64(d2);

		CheckU32CastOverflow(d, i32, L"Overflow in CFixedVector2D::Length() part 2")
		fixed r;
		r.SetInternalValue((i32)d);
		return r;
	}

	/**
	 * Returns -1, 0, +1 depending on whether length is less/equal/greater
	 * than the argument.
	 * Avoids sqrting and overflowing.
	 */
	int CompareLength(fixed cmp) const
	{
		i32 x = X.GetInternalValue(); // abs(x) <= 2^31
		i32 y = Y.GetInternalValue();
		u64 xx = (u64)FIXED_MUL_I64_I32_I32(x, x); // xx <= 2^62
		u64 yy = (u64)FIXED_MUL_I64_I32_I32(y, y);
		u64 d2 = xx + yy; // d2 <= 2^63 (no overflow)

		i32 c = cmp.GetInternalValue();
		u64 c2 = (u64)FIXED_MUL_I64_I32_I32(c, c);
		if (d2 < c2)
			return -1;
		else if (d2 > c2)
			return +1;
		else
			return 0;
	}

	/**
	 * Returns -1, 0, +1 depending on whether length is less/equal/greater
	 * than the argument's length.
	 * Avoids sqrting and overflowing.
	 */
	int CompareLength(const CFixedVector2D& other) const
	{
		i32 x = X.GetInternalValue();
		i32 y = Y.GetInternalValue();
		u64 d2 = (u64)FIXED_MUL_I64_I32_I32(x, x) + (u64)FIXED_MUL_I64_I32_I32(y, y);

		i32 ox = other.X.GetInternalValue();
		i32 oy = other.Y.GetInternalValue();
		u64 od2 = (u64)FIXED_MUL_I64_I32_I32(ox, ox) + (u64)FIXED_MUL_I64_I32_I32(oy, oy);

		if (d2 < od2)
			return -1;
		else if (d2 > od2)
			return +1;
		else
			return 0;
	}

	bool IsZero() const
	{
		return (X.IsZero() && Y.IsZero());
	}

	/**
	 * Normalize the vector so that length is close to 1.
	 * If length is 0, does nothing.
	 */
	void Normalize()
	{
		if (!IsZero())
		{
			fixed l = Length();
			X = X / l;
			Y = Y / l;
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
		}
	}

	/**
	 * Compute the dot product of this vector with another.
	 */
	fixed Dot(const CFixedVector2D& v) const
	{
		i64 x = FIXED_MUL_I64_I32_I32(X.GetInternalValue(), v.X.GetInternalValue());
		i64 y = FIXED_MUL_I64_I32_I32(Y.GetInternalValue(), v.Y.GetInternalValue());
		CheckSignedAdditionOverflow(i64, x, y, L"Overflow in CFixedVector2D::Dot() part 1", L"Underflow in CFixedVector2D::Dot() part 1")
		i64 sum = x + y;
		sum >>= fixed::fract_bits;

		CheckCastOverflow(sum, i32, L"Overflow in CFixedVector2D::Dot() part 2", L"Underflow in CFixedVector2D::Dot() part 2")
		fixed ret;
		ret.SetInternalValue((i32)sum);
		return ret;
	}

	CFixedVector2D Perpendicular() const
	{
		return CFixedVector2D(Y, -X);
	}

	/**
	 * Rotate the vector by the given angle (anticlockwise).
	 */
	CFixedVector2D Rotate(fixed angle) const
	{
		fixed s, c;
		sincos_approx(angle, s, c);
		return CFixedVector2D(X.Multiply(c) + Y.Multiply(s), Y.Multiply(c) - X.Multiply(s));
	}
};

#endif // INCLUDED_FIXED_VECTOR2D
