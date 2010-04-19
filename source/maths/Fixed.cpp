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

// Based on http://www.dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization
CFixed_23_8 atan2_approx(CFixed_23_8 y, CFixed_23_8 x)
{
	CFixed_23_8 zero;

	// Special case to avoid division-by-zero
	if (x.IsZero() && y.IsZero())
		return zero;

	CFixed_23_8 c1;
	c1.SetInternalValue(201); // pi/4 << 8

	CFixed_23_8 c2;
	c2.SetInternalValue(603); // 3*pi/4 << 8

	CFixed_23_8 abs_y = y.Absolute();

	CFixed_23_8 angle;
	if (x >= zero)
	{
		CFixed_23_8 r = (x - abs_y) / (x + abs_y);
		angle = c1 - c1.Multiply(r);
	}
	else
	{
		CFixed_23_8 r = (x + abs_y) / (abs_y - x);
		angle = c2 - c1.Multiply(r);
	}

	if (y < zero)
		return -angle;
	else
		return angle;
}

template<>
CFixed_23_8 CFixed_23_8::Pi()
{
	return CFixed_23_8(804); // = pi * 256
}

void sincos_approx(CFixed_23_8 a, CFixed_23_8& sin_out, CFixed_23_8& cos_out)
{
	// XXX: mustn't use floating-point here - need a fixed-point emulation

	// TODO: it's stupid doing sin/cos with 8-bit precision - we ought to have a CFixed_16_16 instead

	sin_out = CFixed_23_8::FromDouble(sin(a.ToDouble()));
	cos_out = CFixed_23_8::FromDouble(cos(a.ToDouble()));
}
