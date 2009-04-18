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
 * File        : base32.cpp
 * Project     : 0 A.D.
 * Description : base32 conversion
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"


// big endian!
void base32(const size_t in_len, const u8* in, u8* out)
{
	u32 pool = 0;	// of bits from buffer
	size_t pool_bits = 0;	// # bits currently in buffer

	static const u8 tbl[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

	size_t in_bytes_left = in_len;	// to avoid overrunning input buffer
	const size_t out_chars = (in_len*8 + 4) / 5;	// = ceil(# 5-bit blocks)
	for(size_t i = 0; i < out_chars; i++)
	{
		if(pool_bits < 5 && in_bytes_left)
		{
			pool <<= 8;
			pool |= *in++;
			pool_bits += 8;
			in_bytes_left--;
		}

		pool_bits -= 5;
		const size_t c = (pool >> pool_bits) & 31;
		*out++ = tbl[c];
	}

	*out++ = '\0';
}
