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

#include "precompiled.h"

#include "lib.h"
#include "misc.h"


// FNV1-A hash - good for strings.
// if len = 0 (default), treat buf as a C-string;
// otherwise, hash <len> bytes of buf.
u32 fnv_hash(const void* buf, const size_t len)
{
	u32 h = 0x811c9dc5;
		// give distinct values for different length 0 buffers.
		// value taken from FNV; it has no special significance.

	const u8* p = (const u8*)buf;

	// expected case: string
	if(!len)
	{
		while(*p)
		{
			h ^= *p++;
			h *= 0x01000193;
		}
	}
	else
	{
		size_t bytes_left = len;
		while(bytes_left != 0)
		{
			h ^= *p++;
			h *= 0x01000193;

			bytes_left--;
		}
	}

	return h;
}


// FNV1-A hash - good for strings.
// if len = 0 (default), treat buf as a C-string;
// otherwise, hash <len> bytes of buf.
u64 fnv_hash64(const void* buf, const size_t len)
{
	u64 h = 0xCBF29CE484222325;
		// give distinct values for different length 0 buffers.
		// value taken from FNV; it has no special significance.

	const u8* p = (const u8*)buf;

	// expected case: string
	if(!len)
	{
		while(*p)
		{
			h ^= *p++;
			h *= 0x100000001B3;
		}
	}
	else
	{
		size_t bytes_left = len;
		while(bytes_left != 0)
		{
			h ^= *p++;
			h *= 0x100000001B3;

			bytes_left--;
		}
	}

	return h;
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


bool is_pow2(const long n)
{
	return (n != 0) && !(n & (n-1));
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


int ilog2(const float x)
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
	return (u16)MIN(t+y, 0xffff);
}


u16 subusw(u16 x, u16 y)
{
	long t = x;
	return (u16)(MAX(t-y, 0));
}


// provide fminf for non-C99 compilers
#ifndef HAVE_C99

float fminf(float a, float b)
{
	return (a < b)? a : b;
}

#endif



long round(double x)
{
	return (long)(x + 0.5);
}


// input in [0, 1); convert to u8 range
u8 fp_to_u8(double in)
{
	if(!(0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 255;
	}

	int l = round(in * 255.0);
	assert((unsigned int)l <= 255);
	return (u8)l;
}


// input in [0, 1); convert to u16 range
u16 fp_to_u16(double in)
{
	if(!(0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 65535;
	}

	long l = round(in * 65535.0);
	assert((unsigned long)l <= 65535);
	return (u16)l;
}



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
/*
#ifndef _WIN32

char *_itoa(int value, char *out, int radix)
{
	return _ltoa(value, out, radix);
}

static const char digits[]="0123456789abcdef";
	
char *_ultoa(unsigned long int value, char *out, int radix)
{
	char buf[21];
	char *p=buf+21;
	
	do
	{
		*(--p)=digits[value % radix];
		value /= radix;
	}
	while (value);
	
	memcpy(out, p, (buf+21)-p);
	out[(buf+21)-p]=0;
	return out;
}

char *_ltoa(long val, char *out, int radix)
{
	char buf[21];
	char *p=buf+21;
	bool sign=val < 0;
	if (sign) val=-val;
	
	do
	{
		*(--p)=digits[val % radix];
		val /= radix;
	}
	while (val);
	
	if (sign) *(--p) = '-';
	
	memcpy(out, p, (buf+21)-p);
	out[(buf+21)-p]=0;
	return out;
}

#endif
*/

