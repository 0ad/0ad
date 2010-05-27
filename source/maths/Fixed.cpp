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
	// XXX: mustn't use floating-point here - need a fixed-point emulation

	sin_out = CFixed_15_16::FromDouble(sin(a.ToDouble()));
	cos_out = CFixed_15_16::FromDouble(cos(a.ToDouble()));
}
