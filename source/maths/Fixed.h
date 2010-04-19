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

#ifndef INCLUDED_FIXED
#define INCLUDED_FIXED

#include "lib/types.h"
#include "maths/Sqrt.h"

template <typename T>
inline T round_away_from_zero(float value)
{
	return (T)(value >= 0 ? value + 0.5f : value - 0.5f);
}

/**
 * A simple fixed-point number class, with no fancy features
 * like overflow checking or anything. (It has very few basic
 * features too, and needs to be substantially improved before
 * it'll be of much use.)
 *
 * Use CFixed_23_8 rather than using this class directly.
 */
template<typename T, T max_t, int total_bits, int int_bits, int fract_bits_, int fract_pow2>
class CFixed
{
private:
	T value;

	explicit CFixed(T v) : value(v) { }

public:
	enum { fract_bits = fract_bits_ };

	CFixed() : value(0) { }

	static CFixed Zero() { return CFixed(0); }
	static CFixed Pi();

	T GetInternalValue() const { return value; }
	void SetInternalValue(T n) { value = n; }

	// Conversion to/from primitive types:

	static CFixed FromInt(int n)
	{
		return CFixed(n << fract_bits);
	}
	static CFixed FromFloat(float n)
	{
		if (!isfinite(n))
			return CFixed(0);
		float scaled = n * fract_pow2;
		return CFixed(round_away_from_zero<T>(scaled));
	}
	static CFixed FromDouble(double n)
	{
		if (!isfinite(n))
			return CFixed(0);
		double scaled = n * fract_pow2;
		return CFixed(round_away_from_zero<T>(scaled));
	}

	float ToFloat() const
	{
		return value / (float)fract_pow2;
	}
	double ToDouble() const
	{
		return value / (double)fract_pow2;
	}
	int ToInt_RoundToZero() const
	{
		if (value > 0)
			return value >> fract_bits;
		else
			return (value + fract_pow2 - 1) >> fract_bits;
	}

	/// Returns true if the number is precisely 0.
	bool IsZero() const { return value == 0; }

	/// Equality.
	bool operator==(CFixed n) const { return (value == n.value); }

	/// Inequality.
	bool operator!=(CFixed n) const { return (value != n.value); }

	/// Numeric comparison.
	bool operator<=(CFixed n) const { return (value <= n.value); }

	/// Numeric comparison.
	bool operator<(CFixed n) const { return (value < n.value); }

	/// Numeric comparison.
	bool operator>=(CFixed n) const { return (value >= n.value); }

	/// Numeric comparison.
	bool operator>(CFixed n) const { return (value > n.value); }

	// Basic arithmetic:

	/// Add a CFixed. Might overflow.
	CFixed operator+(CFixed n) const { return CFixed(value + n.value); }

	/// Subtract a CFixed. Might overflow.
	CFixed operator-(CFixed n) const { return CFixed(value - n.value); }

	/// Negate a CFixed.
	CFixed operator-() const { return CFixed(-value); }

	/// Divide by a CFixed. Must not have n.IsZero(). Might overflow.
	CFixed operator/(CFixed n) const
	{
		i64 t = (i64)value << fract_bits;
		return CFixed((T)(t / (i64)n.value));
	}

	/// Multiply by an integer. Might overflow.
	CFixed operator*(int n) const { return CFixed(value * n); }

	/// Divide by an integer. Must not have n == 0. Cannot overflow.
	CFixed operator/(int n) const { return CFixed(value / n); }

	CFixed Absolute() const { return CFixed(abs(value)); }

	/**
	 * Multiply by a CFixed. Likely to overflow if both numbers are large,
	 * so we use an ugly name instead of operator* to make it obvious.
	 */
	CFixed Multiply(CFixed n) const
	{
		i64 t = (i64)value * (i64)n.value;
		return CFixed((T)(t >> fract_bits));
	}

	CFixed Sqrt() const
	{
		if (value <= 0)
			return CFixed(0);
		u32 s = isqrt64(value);
		return CFixed((u64)s << (fract_bits / 2));
	}
};

/**
 * A fixed-point number class with 1-bit sign, 23-bit integral part, 8-bit fractional part.
 */
typedef CFixed<i32, (i32)0x7fffffff, 32, 23, 8, 256> CFixed_23_8;

/**
 * Inaccurate approximation of atan2 over fixed-point numbers.
 * Maximum error is almost 0.08 radians (4.5 degrees).
 */
CFixed_23_8 atan2_approx(CFixed_23_8 y, CFixed_23_8 x);

void sincos_approx(CFixed_23_8 a, CFixed_23_8& sin_out, CFixed_23_8& cos_out);

#endif // INCLUDED_FIXED
