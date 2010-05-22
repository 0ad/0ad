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

	// TODO: this could be made more precise

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

	// Integer part
	while (true)
	{
		if (*c == '.')
		{
			++c;
			// Fractional part
			u32 div = 10;
			while (true)
			{
				if (*c >= '0' && *c <= '9')
				{
					r += CFixed_15_16::FromInt(*c - '0') / (int)div;
					++c;
					div *= 10;
					if (div > 65536)
						break; // any further digits will be too small to have any effect
				}
				else
				{
					// invalid character or end of string
					break;
				}
			}
			break;
		}
		else if (*c >= '0' && *c <= '9')
		{
			r = r * 10; // TODO: handle overflow gracefully, maybe
			r += CFixed_15_16::FromInt(*c - '0');
			++c;
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
