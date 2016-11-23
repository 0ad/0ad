/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * base32 conversion
 */

#include "precompiled.h"


// big endian!
void base32(const size_t in_len, const u8* in, u8* out)
{
	u32 pool = 0;	// of bits from buffer
	ssize_t pool_bits = 0;	// # bits currently in buffer

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
		size_t c;
		if (pool_bits < 0)
			c = (pool << -pool_bits) & 31;
		else
			c = (pool >> pool_bits) & 31;
		*out++ = tbl[c];
	}

	*out++ = '\0';
}
