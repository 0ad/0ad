// miscellany
//
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/


#include <cassert>
#include <cstdlib>
#include <cmath>

#include "time.h"
#include "misc.h"


#include <map>



// FNV1-A
u32 fnv_hash(const char* str, size_t len)
{
	u32 h = 0;

	while(len--)
	{
		h ^= *(const u8*)str++;
		h *= 0x01000193;
	}

	return h;
}




u16 bswap16(u16 x)
{
	return (u16)(((x & 0xff) << 8) | (x >> 8));
}


u32 bswap32(u32 x)
{
#ifdef _M_IX86

__asm
{
	mov		eax, [x]
	bswap	eax
	mov		[x], eax
}

#else

	u32 t = x;

	for(int i = 0; i < 4; i++)
	{
		x <<= 8;
		x |= t & 0xff;
	}

#endif
	return x;
}


void bswap32(const u8* data, int cnt)
{
#ifdef _M_IX86

	UNUSED(data)
	UNUSED(cnt)

	__asm
	{
		mov		edx, [data]
		mov		ecx, [cnt]
$loop:	mov		eax, [edx]
		bswap	eax
		mov		[edx], eax
		add		edx, 4
		dec		ecx
		jnz		$loop
	}

#else

	u32* p = (u32*)data;
	for(int i = 0; i < cnt; i++, p++)
		*p = bswap32(*p);

#endif
}


bool is_pow2(const int n)
{
	return (n > 0) && !(n & (n-1));
}


// return -1 if not an integral power of 2,
// otherwise the base2 logarithm

int ilog2(const int n)
{
#ifdef _M_IX86

	__asm
	{
		mov		ecx, [n]
		or		eax, -1			// return value
		lea		edx, [ecx-1]
		test	ecx, edx		// power of 2?
		jnz		$ret
		bsf		eax, ecx
	$ret:
		mov		[n], eax
	}

	return n;

#else

	if(n || n & (n-1))
		return -1;

	int i = 1, j = 0;
	for(; i != n; i += i, j++)
		;
	return j;

#endif
}


static int ilog2(const float x)
{
	u32 i = (u32&)x;
	u32 exp = (i >> 23) & 0xff;
	return (int)exp - 127;
}


uintptr_t round_up(uintptr_t val, uintptr_t multiple)
{
	val += multiple-1;
	val -= val % multiple;
	return val;
}


u16 addusw(u16 x, u16 y)
{
	u32 t = x;
	return min(t+y, 0xffff);
}


u16 subusw(u16 x, u16 y)
{
	long t = x;
	return max(t-y, 0);
}



// provide fminf for non-C99 compilers
#ifndef HAVE_C99

float fminf(float a, float b)
{
	return (a < b)? a : b;
}

#endif


// replaces pathetic MS libc implementation
#if defined(_M_IX86) && defined(_WIN32)

double _ceil(double f)
{
	double r;

	const float _49 = 0.499999f;
	__asm
	{
		fld			[f]
		fadd		[_49]
		frndint
		fstp		[r]
	}

	UNUSED(f)

	return r;
}

#endif


// big endian!
void base32(const int len, const u8* in, u8* out)
{
	int bits = 0;
	u32 pool = 0;

	static u8 tbl[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

	for(int i = 0; i < len; i++)
	{
		if(bits < 5)
		{
			pool <<= 8;
			pool |= *in++;
			bits += 8;
		}

		bits -= 5;
		int c = (pool >> bits) & 31;
		*out++ = tbl[c];
	}
}
