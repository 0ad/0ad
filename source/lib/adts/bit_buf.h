/* Copyright (c) 2011 Wildfire Games
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
 * FIFO bit queue
 */

#ifndef INCLUDED_ADTS_BIT_BUF
#define INCLUDED_ADTS_BIT_BUF

#include "lib/bits.h"

struct BitBuf
{
	uintptr_t buf;
	uintptr_t cur;	// bit to be appended (toggled by add())
	size_t len;	// |buf| [bits]

	void reset()
	{
		buf = 0;
		cur = 0;
		len = 0;
	}

	// toggle current bit if desired, and add to buffer (new bit is LSB)
	void add(uintptr_t toggle)
	{
		cur ^= toggle;
		buf <<= 1;
		buf |= cur;
		len++;
	}

	// extract LS n bits
	size_t extract(uintptr_t n)
	{
		const uintptr_t bits = buf & bit_mask<uintptr_t>(n);
		buf >>= n;

		return bits;
	}
};

#endif	// #ifndef INCLUDED_ADTS_BIT_BUF
