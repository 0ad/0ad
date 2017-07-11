/* Copyright (C) 2010 Wildfire Games.
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
 * byte order (endianness) support routines.
 */

#include "precompiled.h"
#include "lib/byte_order.h"

#include "lib/bits.h"

#ifndef swap16
u16 swap16(const u16 x)
{
	return (u16)(((x & 0xff) << 8) | (x >> 8));
}
#endif

#ifndef swap32
u32 swap32(const u32 x)
{
	return (x << 24) |
		(x >> 24) |
		((x << 8) & 0x00ff0000) |
		((x >> 8) & 0x0000ff00);
}
#endif

#ifndef swap64
u64 swap64(const u64 x)
{
	const u32 lo = (u32)(x & 0xFFFFFFFF);
	const u32 hi = (u32)(x >> 32);
	u64 ret = swap32(lo);
	ret <<= 32;
	// careful: must shift var of type u64, not u32
	ret |= swap32(hi);
	return ret;
}
#endif


//-----------------------------------------------------------------------------


u16 read_le16(const void* p)
{
	u16 n;
	memcpy(&n, p, sizeof(n));
	return to_le16(n);
}

u32 read_le32(const void* p)
{
	u32 n;
	memcpy(&n, p, sizeof(n));
	return to_le32(n);
}

u64 read_le64(const void* p)
{
	u64 n;
	memcpy(&n, p, sizeof(n));
	return to_le64(n);
}


u16 read_be16(const void* p)
{
	u16 n;
	memcpy(&n, p, sizeof(n));
	return to_be16(n);
}

u32 read_be32(const void* p)
{
	u32 n;
	memcpy(&n, p, sizeof(n));
	return to_be32(n);
}

u64 read_be64(const void* p)
{
	u64 n;
	memcpy(&n, p, sizeof(n));
	return to_be64(n);
}


void write_le16(void* p, u16 x)
{
	u16 n = to_le16(x);
	memcpy(p, &n, sizeof(n));
}

void write_le32(void* p, u32 x)
{
	u32 n = to_le32(x);
	memcpy(p, &n, sizeof(n));
}

void write_le64(void* p, u64 x)
{
	u64 n = to_le64(x);
	memcpy(p, &n, sizeof(n));
}


void write_be16(void* p, u16 x)
{
	u16 n = to_be16(x);
	memcpy(p, &n, sizeof(n));
}

void write_be32(void* p, u32 x)
{
	u32 n = to_be32(x);
	memcpy(p, &n, sizeof(n));
}

void write_be64(void* p, u64 x)
{
	u64 n = to_be64(x);
	memcpy(p, &n, sizeof(n));
}


u64 movzx_le64(const u8* p, size_t size_bytes)
{
	u64 number = 0;
	for(size_t i = 0; i < std::min(size_bytes, (size_t)8u); i++)
		number |= ((u64)p[i]) << (i*8);

	return number;
}

u64 movzx_be64(const u8* p, size_t size_bytes)
{
	u64 number = 0;
	for(size_t i = 0; i < std::min(size_bytes, (size_t)8u); i++)
	{
		number <<= 8;
		number |= p[i];
	}

	return number;
}


static inline i64 SignExtend(u64 bits, size_t size_bytes)
{
	// no point in sign-extending if >= 8 bytes were requested
	if(size_bytes < 8)
	{
		const u64 sign_bit = Bit<u64>((size_bytes*8)-1);

		// number would be negative in the smaller type,
		// so sign-extend, i.e. set all more significant bits.
		if(bits & sign_bit)
		{
			const u64 valid_bit_mask = (sign_bit+sign_bit)-1;
			bits |= ~valid_bit_mask;
		}
	}

	const i64 number = static_cast<i64>(bits);
	return number;
}

i64 movsx_le64(const u8* p, size_t size_bytes)
{
	const u64 number = movzx_le64(p, size_bytes);
	return SignExtend(number, size_bytes);
}

i64 movsx_be64(const u8* p, size_t size_bytes)
{
	const u64 number = movzx_be64(p, size_bytes);
	return SignExtend(number, size_bytes);
}
