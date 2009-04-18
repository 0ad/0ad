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
 * File        : bits.cpp
 * Project     : 0 A.D.
 * Description : bit-twiddling.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "bits.h"

static inline u32 get_float_bits(const float x)
{
	u32 ret;
	memcpy(&ret, &x, 4);
	return ret;
}

int floor_log2(const float x)
{
	const u32 i = get_float_bits(x);
	const u32 biased_exp = (i >> 23) & 0xFF;
	return (int)biased_exp - 127;
}
