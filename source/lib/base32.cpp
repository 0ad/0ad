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
