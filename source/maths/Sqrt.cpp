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

#include "Sqrt.h"

// Based on http://freaknet.org/martin/tape/gos/misc/personal/msc/sqrt/sqrt.html
u32 isqrt64(u64 n)
{
	u64 op = n;
	u64 res = 0;
	u64 one = (u64)1 << 62; // highest power of four <= than the argument

	while (one > op)
		one >>= 2;

	while (one != 0)
	{
		if (op >= res + one)
		{
			op -= (res + one);
			res += (one << 1);
		}
		res >>= 1;
		one >>= 2;
	}
	return (u32)res;
}

// TODO: This should be equivalent to (u32)sqrt((double)n), and in practice
// that seems to be true for all input, so do we actually need this integer-only
// implementation? i.e. are there any platforms / compiler settings where
// sqrt(double) won't give the correct answer? and is sqrt(double) faster?
