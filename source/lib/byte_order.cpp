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
 * File        : byte_order.cpp
 * Project     : 0 A.D.
 * Description : byte order (endianness) support routines.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "byte_order.h"

#include "bits.h"

#ifndef swap16

u16 swap16(const u16 x)
{
	return (u16)(((x & 0xff) << 8) | (x >> 8));
}

u32 swap32(const u32 x)
{
	return (x << 24) |
		(x >> 24) |
		((x << 8) & 0x00ff0000) |
		((x >> 8) & 0x0000ff00);
}

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

#endif	// #ifndef swap16


//-----------------------------------------------------------------------------

u16 to_le16(u16 x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return swap16(x);
#else
	return x;
#endif
}

u32 to_le32(u32 x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return swap32(x);
#else
	return x;
#endif
}

u64 to_le64(u64 x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return swap64(x);
#else
	return x;
#endif
}


u16 to_be16(u16 x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return x;
#else
	return swap16(x);
#endif
}

u32 to_be32(u32 x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return x;
#else
	return swap32(x);
#endif
}

u64 to_be64(u64 x)
{
#if BYTE_ORDER == BIG_ENDIAN
	return x;
#else
	return swap64(x);
#endif
}


u16 read_le16(const void* p)
{
	return to_le16(*(u16*)p);
}

u32 read_le32(const void* p)
{
	return to_le32(*(u32*)p);
}

u64 read_le64(const void* p)
{
	return to_le64(*(u64*)p);
}


u16 read_be16(const void* p)
{
	return to_be16(*(u16*)p);
}

u32 read_be32(const void* p)
{
	return to_be32(*(u32*)p);
}

u64 read_be64(const void* p)
{
	return to_be64(*(u64*)p);
}


void write_le16(void* p, u16 x)
{
	*(u16*)p = to_le16(x);
}

void write_le32(void* p, u32 x)
{
	*(u32*)p = to_le32(x);
}

void write_le64(void* p, u64 x)
{
	*(u64*)p = to_le64(x);
}


void write_be16(void* p, u16 x)
{
	*(u16*)p = to_be16(x);
}

void write_be32(void* p, u32 x)
{
	*(u32*)p = to_be32(x);
}

void write_be64(void* p, u64 x)
{
	*(u64*)p = to_be64(x);
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
