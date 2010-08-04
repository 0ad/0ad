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

#include "precompiled.h"

#include "Fixed.h"

#include "ps/CStr.h"

template<>
CFixed_15_16 CFixed_15_16::FromString(const CStr8& s)
{
	// Parse a superset of the xsd:decimal syntax: [-+]?\d*(\.\d*)?

	if (s.empty())
		return CFixed_15_16::Zero();

	bool neg = false;
	CFixed_15_16 r;
	const char* c = &s[0];

	if (*c == '+')
	{
		++c;
	}
	else if (*c == '-')
	{
		++c;
		neg = true;
	}

	while (true)
	{
		// Integer part:
		if (*c >= '0' && *c <= '9')
		{
			r = r * 10; // TODO: handle overflow gracefully, maybe
			r += CFixed_15_16::FromInt(*c - '0');
			++c;
		}
		else if (*c == '.')
		{
			++c;
			u32 frac = 0;
			u32 div = 1;
			// Fractional part
			while (*c >= '0' && *c <= '9')
			{
				frac *= 10;
				frac += (*c - '0');
				div *= 10;
				++c;
				if (div >= 100000)
				{
					// any further digits will be too small to have any major effect
					break;
				}
			}
			// too many digits or invalid character or end of string - add the fractional part and stop
			r += CFixed_15_16(((u64)frac << 16) / div);
			break;
		}
		else
		{
			// invalid character or end of string
			break;
		}
	}

	return (neg ? -r : r);
}

template<>
CFixed_15_16 CFixed_15_16::FromString(const CStrW& s)
{
	return FromString(CStr8(s));
}

template<>
CStr8 CFixed_15_16::ToString() const
{
	std::stringstream r;

	u32 posvalue = abs(value);
	if (value < 0)
		r << "-";

	r << (posvalue >> fract_bits);

	u16 fraction = posvalue & ((1 << fract_bits) - 1);
	if (fraction)
	{
		r << ".";

		u32 frac = 0;
		u32 div = 1;

		// Do the inverse of FromString: Keep adding digits until (frac<<16)/div == expected fraction
		while (true)
		{
			frac *= 10;
			div *= 10;

			// Low estimate of d such that ((frac+d)<<16)/div == fraction
			u32 digit = (((u64)fraction*div) >> 16) - frac;
			frac += digit;

			// If this gives the exact target, then add the digit and stop
			if (((u64)frac << 16) / div == fraction)
			{
				r << digit;
				break;
			}

			// If the next higher digit gives the exact target, then add that digit and stop
			if (digit <= 8 && (((u64)frac+1) << 16) / div == fraction)
			{
				r << digit+1;
				break;
			}

			// Otherwise add the digit and continue
			r << digit;
		}
	}

	return r.str();
}

// Based on http://www.dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization
CFixed_15_16 atan2_approx(CFixed_15_16 y, CFixed_15_16 x)
{
	CFixed_15_16 zero;

	// Special case to avoid division-by-zero
	if (x.IsZero() && y.IsZero())
		return zero;

	CFixed_15_16 c1;
	c1.SetInternalValue(51472); // pi/4 << 16

	CFixed_15_16 c2;
	c2.SetInternalValue(154415); // 3*pi/4 << 16

	CFixed_15_16 abs_y = y.Absolute();

	CFixed_15_16 angle;
	if (x >= zero)
	{
		CFixed_15_16 r = (x - abs_y) / (x + abs_y);
		angle = c1 - c1.Multiply(r);
	}
	else
	{
		CFixed_15_16 r = (x + abs_y) / (abs_y - x);
		angle = c2 - c1.Multiply(r);
	}

	if (y < zero)
		return -angle;
	else
		return angle;
}

template<>
CFixed_15_16 CFixed_15_16::Pi()
{
	return CFixed_15_16(205887); // = pi << 16
}

void sincos_approx(CFixed_15_16 a, CFixed_15_16& sin_out, CFixed_15_16& cos_out)
{
	// Based on http://www.coranac.com/2009/07/sines/

	// TODO: this could be made a bit more precise by being careful about scaling

	typedef CFixed_15_16 fixed;

	fixed c2_pi;
	c2_pi.SetInternalValue(41721); // = 2/pi << 16

	// Map radians onto the range [0, 4)
	fixed z = a.Multiply(c2_pi) % fixed::FromInt(4);

	// Map z onto the range [-1, +1] for sin, and the same with z+1 to compute cos
	fixed sz, cz;
	if (z >= fixed::FromInt(3)) // [3, 4)
	{
		sz = z - fixed::FromInt(4);
		cz = z - fixed::FromInt(3);
	}
	else if (z >= fixed::FromInt(2)) // [2, 3)
	{
		sz = fixed::FromInt(2) - z;
		cz = z - fixed::FromInt(3);
	}
	else if (z >= fixed::FromInt(1)) // [1, 2)
	{
		sz = fixed::FromInt(2) - z;
		cz = fixed::FromInt(1) - z;
	}
	else // [0, 1)
	{
		sz = z;
		cz = fixed::FromInt(1) - z;
	}

	// Third-order (max absolute error ~0.02)

//	sin_out = (sz / 2).Multiply(fixed::FromInt(3) - sz.Multiply(sz));
//	cos_out = (cz / 2).Multiply(fixed::FromInt(3) - cz.Multiply(cz));

	// Fifth-order (max absolute error ~0.0005)

	fixed sz2 = sz.Multiply(sz);
	sin_out = sz.Multiply(fixed::Pi() - sz2.Multiply(fixed::Pi()*2 - fixed::FromInt(5) - sz2.Multiply(fixed::Pi() - fixed::FromInt(3)))) / 2;

	fixed cz2 = cz.Multiply(cz);
	cos_out = cz.Multiply(fixed::Pi() - cz2.Multiply(fixed::Pi()*2 - fixed::FromInt(5) - cz2.Multiply(fixed::Pi() - fixed::FromInt(3)))) / 2;
}
