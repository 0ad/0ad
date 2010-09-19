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

class CStr8;
class CStrW;

#ifndef NDEBUG
#define USE_FIXED_OVERFLOW_CHECKS
#endif // NDEBUG

//define overflow macros
#ifndef USE_FIXED_OVERFLOW_CHECKS

#define CheckSignedSubtractionOverflow(type, left, right, overflowWarning, underflowWarning)
#define CheckSignedAdditionOverflow(type, left, right, overflowWarning, underflowWarning)
#define CheckCastOverflow(var, targetType, overflowWarning, underflowWarning)
#define CheckU32CastOverflow(var, targetType, overflowWarning)
#define CheckUnsignedAdditionOverflow(result, operand, overflowWarning)
#define CheckUnsignedSubtractionOverflow(result, operand, overflowWarning)
#define CheckNegationOverflow(var, type, overflowWarning)
#define CheckMultiplicationOverflow(type, left, right, overflowWarning, underflowWarning)
#define CheckDivisionOverflow(type, left, right, overflowWarning)

#else // USE_FIXED_OVERFLOW_CHECKS

#define CheckSignedSubtractionOverflow(type, left, right, overflowWarning, underflowWarning) \
	if(left > 0 && right < 0 && left > std::numeric_limits<type>::max() + right) \
		debug_warn(overflowWarning); \
	else if(left < 0 && right > 0 && left < std::numeric_limits<type>::min() + right) \
		debug_warn(underflowWarning);

#define CheckSignedAdditionOverflow(type, left, right, overflowWarning, underflowWarning) \
	if(left > 0 && right > 0 && std::numeric_limits<type>::max() - left < right) \
		debug_warn(overflowWarning); \
	else if(left < 0 && right < 0 && std::numeric_limits<type>::min() - left > right) \
		debug_warn(underflowWarning);

#define CheckCastOverflow(var, targetType, overflowWarning, underflowWarning) \
	if(var > std::numeric_limits<targetType>::max()) \
		debug_warn(overflowWarning); \
	else if(var < std::numeric_limits<targetType>::min()) \
		debug_warn(underflowWarning);

#define CheckU32CastOverflow(var, targetType, overflowWarning) \
	if(var > (u32)std::numeric_limits<targetType>::max()) \
		debug_warn(overflowWarning);

#define CheckUnsignedAdditionOverflow(result, operand, overflowWarning) \
	if(result < operand) \
		debug_warn(overflowWarning);

#define CheckUnsignedSubtractionOverflow(result, left, overflowWarning) \
	if(result > left) \
		debug_warn(overflowWarning);

#define CheckNegationOverflow(var, type, overflowWarning) \
	if(value == std::numeric_limits<type>::min()) \
		debug_warn(overflowWarning);

#define CheckMultiplicationOverflow(type, left, right, overflowWarning, underflowWarning) \
	i64 res##left = (i64)left * (i64)right; \
	CheckCastOverflow(res##left, type, overflowWarning, underflowWarning)

#define CheckDivisionOverflow(type, left, right, overflowWarning) \
	if(right == -1) { CheckNegationOverflow(left, type, overflowWarning) }

#endif // USE_FIXED_OVERFLOW_CHECKS

template <typename T>
inline T round_away_from_zero(float value)
{
	return (T)(value >= 0 ? value + 0.5f : value - 0.5f);
}

template <typename T>
inline T round_away_from_zero(double value)
{
	return (T)(value >= 0 ? value + 0.5 : value - 0.5);
}

/**
 * A simple fixed-point number class.
 *
 * Use 'fixed' rather than using this class directly.
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

	static CFixed FromString(const CStr8& s);
	static CFixed FromString(const CStrW& s);

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

	int ToInt_RoundToInfinity() const
	{
		return (value + fract_pow2 - 1) >> fract_bits;
	}

	int ToInt_RoundToNegInfinity() const
	{
		return value >> fract_bits;
	}

	/// Returns the shortest string such that FromString will parse to the correct value.
	CStr8 ToString() const;

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
	CFixed operator+(CFixed n) const
	{
		CheckSignedAdditionOverflow(T, value, n.value, L"Overflow in CFixed::operator+(CFixed n)", L"Underflow in CFixed::operator+(CFixed n)")
		return CFixed(value + n.value);
	}

	/// Subtract a CFixed. Might overflow.
	CFixed operator-(CFixed n) const
	{
		CheckSignedSubtractionOverflow(T, value, n.value, L"Overflow in CFixed::operator-(CFixed n)", L"Underflow in CFixed::operator-(CFixed n)")
		return CFixed(value - n.value);
	}

	/// Add a CFixed. Might overflow.
	CFixed& operator+=(CFixed n) { *this = *this + n; return *this; }

	/// Subtract a CFixed. Might overflow.
	CFixed& operator-=(CFixed n) { *this = *this - n; return *this; }

	/// Negate a CFixed.
	CFixed operator-() const
	{
		CheckNegationOverflow(value, T, L"Overflow in CFixed::operator-()")
		return CFixed(-value);
	}

	/// Divide by a CFixed. Must not have n.IsZero(). Might overflow.
	CFixed operator/(CFixed n) const
	{
		i64 t = (i64)value << fract_bits;
		i64 result = t / (i64)n.value;

		CheckCastOverflow(result, T, L"Overflow in CFixed::operator/(CFixed n)", L"Underflow in CFixed::operator/(CFixed n)")
		return CFixed((T)result);
	}

	/// Multiply by an integer. Might overflow.
	CFixed operator*(int n) const
	{
		CheckMultiplicationOverflow(T, value, n, L"Overflow in CFixed::operator*(int n)", L"Underflow in CFixed::operator*(int n)")
		return CFixed(value * n);
	}

	/// Divide by an integer. Must not have n == 0. Cannot overflow unless n == -1.
	CFixed operator/(int n) const
	{
		CheckDivisionOverflow(T, value, n, L"Overflow in CFixed::operator/(int n)")
		return CFixed(value / n);
	}

	/// Mod by a fixed. Must not have n == 0. Result has the same sign as n.
	CFixed operator%(CFixed n) const
	{
		T t = value % n.value;
		if (n.value > 0 && t < 0)
			t += n.value;
		else if (n.value < 0 && t > 0)
			t += n.value;

		return CFixed(t);
	}

	CFixed Absolute() const { return CFixed(abs(value)); }

	/**
	 * Multiply by a CFixed. Likely to overflow if both numbers are large,
	 * so we use an ugly name instead of operator* to make it obvious.
	 */
	CFixed Multiply(CFixed n) const
	{
		i64 t = (i64)value * (i64)n.value;
		t >>= fract_bits;

		CheckCastOverflow(t, T, L"Overflow in CFixed::Multiply(CFixed n)", L"Underflow in CFixed::Multiply(CFixed n)")
		return CFixed((T)t);
	}

	/**
	 * Compute this*m/d. Must not have d == 0. Won't overflow if the result can be represented as a CFixed.
	 */
	CFixed MulDiv(CFixed m, CFixed d) const
	{
		i64 t = ((i64)value * (i64)m.value) / (i64)d.value;
		CheckCastOverflow(t, T, L"Overflow in CFixed::Multiply(CFixed n)", L"Underflow in CFixed::Multiply(CFixed n)")
		return CFixed((T)t);
	}

	CFixed Sqrt() const
	{
		if (value <= 0)
			return CFixed(0);
		u32 s = isqrt64((u64)value << fract_bits);
		return CFixed(s);
	}

private:
	// Prevent dangerous accidental implicit conversions of floats to ints in certain operations
	CFixed operator*(float n) const;
	CFixed operator/(float n) const;
};

/**
 * A fixed-point number class with 1-bit sign, 15-bit integral part, 16-bit fractional part.
 */
typedef CFixed<i32, (i32)0x7fffffff, 32, 15, 16, 65536> CFixed_15_16;

/**
 * Default fixed-point type used by the engine.
 */
typedef CFixed_15_16 fixed;

namespace std
{
/**
 * std::numeric_limits specialisation, currently just providing min and max
 */
template<typename T, T max_t, int total_bits, int int_bits, int fract_bits_, int fract_pow2>
struct numeric_limits<CFixed<T, max_t, total_bits, int_bits, fract_bits_, fract_pow2> >
{
	typedef CFixed<T, max_t, total_bits, int_bits, fract_bits_, fract_pow2> fixed;
public:
	static const bool is_specialized = true;
	static fixed min() throw() { fixed f; f.SetInternalValue(std::numeric_limits<T>::min()); return f; }
	static fixed max() throw() { fixed f; f.SetInternalValue(std::numeric_limits<T>::max()); return f; }
};
}

/**
 * Inaccurate approximation of atan2 over fixed-point numbers.
 * Maximum error is almost 0.08 radians (4.5 degrees).
 */
CFixed_15_16 atan2_approx(CFixed_15_16 y, CFixed_15_16 x);

/**
 * Compute sin(a) and cos(a).
 * Maximum error for -2pi < a < 2pi is almost 0.0005.
 */
void sincos_approx(CFixed_15_16 a, CFixed_15_16& sin_out, CFixed_15_16& cos_out);

#endif // INCLUDED_FIXED
