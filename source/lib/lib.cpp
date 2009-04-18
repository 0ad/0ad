/* Copyright (C) 2009 Wildfire Games.
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

/**
 * =========================================================================
 * File        : lib.cpp
 * Project     : 0 A.D.
 * Description : various utility functions.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "lib.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib/app_hooks.h"
#include "lib/sysdep/sysdep.h"


u16 addusw(u16 x, u16 y)
{
	u32 t = x;
	return (u16)std::min(t+y, 0xFFFFu);
}

u16 subusw(u16 x, u16 y)
{
	long t = x;
	return (u16)(std::max(t-y, 0l));
}


//-----------------------------------------------------------------------------
// type conversion

// these avoid a common mistake in using >> (ANSI requires shift count be
// less than the bit width of the type).

u32 u64_hi(u64 x)
{
	return (u32)(x >> 32);
}

u32 u64_lo(u64 x)
{
	return (u32)(x & 0xFFFFFFFF);
}

u16 u32_hi(u32 x)
{
	return (u16)(x >> 16);
}

u16 u32_lo(u32 x)
{
	return (u16)(x & 0xFFFF);
}


u64 u64_from_u32(u32 hi, u32 lo)
{
	u64 x = (u64)hi;
	x <<= 32;
	x |= lo;
	return x;
}

u32 u32_from_u16(u16 hi, u16 lo)
{
	u32 x = (u32)hi;
	x <<= 16;
	x |= lo;
	return x;
}


// input in [0, 1); convert to u8 range
u8 u8_from_double(double in)
{
	if(!(0.0 <= in && in < 1.0))
	{
		debug_assert(0);	// clampf not in [0,1)
		return 255;
	}

	int l = (int)(in * 255.0);
	debug_assert((unsigned)l <= 255u);
	return (u8)l;
}

// input in [0, 1); convert to u16 range
u16 u16_from_double(double in)
{
	if(!(0.0 <= in && in < 1.0))
	{
		debug_assert(0);	// clampf not in [0,1)
		return 65535;
	}

	long l = (long)(in * 65535.0);
	debug_assert((unsigned long)l <= 65535u);
	return (u16)l;
}
