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


#include <stdlib.h>
#include <string.h>

#include "lib/types.h"
#include "lib.h"

#include "sysdep/sysdep.h"


// FNV1-A hash - good for strings.
// if len = 0 (default), treat buf as a C-string;
// otherwise, hash <len> bytes of buf.
u32 fnv_hash(const void* buf, const size_t len)
{
	u32 h = 0x811c9dc5u;
		// give distinct values for different length 0 buffers.
		// value taken from FNV; it has no special significance.

	const u8* p = (const u8*)buf;

	// expected case: string
	if(!len)
	{
		while(*p)
		{
			h ^= *p++;
			h *= 0x01000193u;
		}
	}
	else
	{
		size_t bytes_left = len;
		while(bytes_left != 0)
		{
			h ^= *p++;
			h *= 0x01000193u;

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
	u64 h = 0xCBF29CE484222325ull;
		// give distinct values for different length 0 buffers.
		// value taken from FNV; it has no special significance.

	const u8* p = (const u8*)buf;

	// expected case: string
	if(!len)
	{
		while(*p)
		{
			h ^= *p++;
			h *= 0x100000001B3ull;
		}
	}
	else
	{
		size_t bytes_left = len;
		while(bytes_left != 0)
		{
			h ^= *p++;
			h *= 0x100000001B3ull;

			bytes_left--;
		}
	}

	return h;
}


// special version for strings: first converts to lowercase
// (useful for comparing mixed-case filenames).
// note: still need <len>, e.g. to support non-0-terminated strings
u32 fnv_lc_hash(const char* str, const size_t len)
{
	u32 h = 0x811c9dc5u;
		// give distinct values for different length 0 buffers.
		// value taken from FNV; it has no special significance.

	// expected case: string
	if(!len)
	{
		while(*str)
		{
			h ^= tolower(*str++);
			h *= 0x01000193u;
		}
	}
	else
	{
		size_t bytes_left = len;
		while(bytes_left != 0)
		{
			h ^= tolower(*str++);
			h *= 0x01000193u;

			bytes_left--;
		}
	}

	return h;
}





bool is_pow2(const long n)
{
	return (n != 0l) && !(n & (n-1l));
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
		jnz		skip
		bsf		eax, ecx
	skip:
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


// return log base 2, rounded up.
uint log2(uint x)
{
	uint bit = 1;
	uint l = 0;
	while(bit < x)
	{
		l++;
		bit += bit;
	}

	return l;
}


int ilog2(const float x)
{
	const u32 i = (u32&)x;
	u32 biased_exp = (i >> 23) & 0xff;
	return (int)biased_exp - 127;
}


// multiple must be a power of two.
uintptr_t round_up(const uintptr_t n, const uintptr_t multiple)
{
	debug_assert(is_pow2((long)multiple));	// also catches divide-by-zero
	const uintptr_t result = (n + multiple-1) & ~(multiple-1);
	debug_assert(n <= result && result < n+multiple);
	return result;
}


u16 addusw(u16 x, u16 y)
{
	u32 t = x;
	return (u16)MIN(t+y, 0xffffu);
}


u16 subusw(u16 x, u16 y)
{
	long t = x;
	return (u16)(MAX(t-y, 0));
}

// zero-extend <size> (truncated to 8) bytes of little-endian data to u64,
// starting at address <p> (need not be aligned).
u64 movzx_64le(const u8* p, size_t size)
{
	if(size > 8)
		size = 8;

	u64 data = 0;
	for(u64 i = 0; i < MIN(size,8); i++)
		data |= ((u64)p[i]) << (i*8);

	return data;
}


// sign-extend <size> (truncated to 8) bytes of little-endian data to i64,
// starting at address <p> (need not be aligned).
i64 movsx_64le(const u8* p, size_t size)
{
	if(size > 8)
		size = 8;

	u64 data = movzx_64le(p, size);

	// no point in sign-extending if >= 8 bytes were requested
	if(size < 8)
	{
		u64 sign_bit = 1;
		sign_bit <<= (size*8)-1;
		// be sure that we don't shift more than variable's bit width

		// number would be negative in the smaller type,
		// so sign-extend, i.e. set all more significant bits.
		if(data & sign_bit)
		{
			const u64 size_mask = (sign_bit+sign_bit)-1;
			data |= ~size_mask;
		}
	}

	return (i64)data;
}



// input in [0, 1); convert to u8 range
u8 fp_to_u8(double in)
{
	if(!(0.0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 255;
	}

	int l = (int)(in * 255.0);
	debug_assert((unsigned int)l <= 255u);
	return (u8)l;
}


// input in [0, 1); convert to u16 range
u16 fp_to_u16(double in)
{
	if(!(0.0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 65535;
	}

	long l = (long)(in * 65535.0);
	debug_assert((unsigned long)l <= 65535u);
	return (u16)l;
}





// return random integer in [0, limit).
// does not use poorly distributed lower bits of rand().
int rand_up_to(int limit)
{
	// (i64 avoids overflowing in multiply)
	const i64 ret = ((i64)limit * rand()) / (RAND_MAX+1);
	debug_assert(0 <= ret && ret < limit);
	return (int)ret;
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






// case-insensitive check if string <s> matches the pattern <w>,
// which may contain '?' or '*' wildcards. if so, return 1, otherwise 0.
// idea from http://www.codeproject.com/string/wildcmp.asp .
// note: NULL wildcard pattern matches everything!
int match_wildcard(const char* s, const char* w)
{
	if(!w)
		return 1;

	// saved position in both strings, used to expand '*':
	// s2 is advanced until match.
	// initially 0 - we abort on mismatch before the first '*'.
	const char* s2 = 0;
	const char* w2 = 0;

	while(*s)
	{
		const int wc = *w;
		if(wc == '*')
		{
			// wildcard string ended with * => match.
			if(*++w == '\0')
				return 1;

			w2 = w;
			s2 = s+1;
		}
		// match one character
		else if(toupper(wc) == toupper(*s) || wc == '?')
		{
			w++;
			s++;
		}
		// mismatched character
		else
		{
			// no '*' found yet => mismatch.
			if(!s2)
				return 0;

			// resume at previous position+1
			w = w2;
			s = s2++;
		}
	}

	// strip trailing * in wildcard string
	while(*w == '*')
		w++;

	return (*w == '\0');
}

int match_wildcardw(const wchar_t* s, const wchar_t* w)
{
	if(!w)
		return 1;

	// saved position in both strings, used to expand '*':
	// s2 is advanced until match.
	// initially 0 - we abort on mismatch before the first '*'.
	const wchar_t* s2 = 0;
	const wchar_t* w2 = 0;

	while(*s)
	{
		const wchar_t wc = *w;
		if(wc == '*')
		{
			// wildcard string ended with * => match.
			if(*++w == '\0')
				return 1;

			w2 = w;
			s2 = s+1;
		}
		// match one character
		else if(towupper(wc) == towupper(*s) || wc == '?')
		{
			w++;
			s++;
		}
		// mismatched character
		else
		{
			// no '*' found yet => mismatch.
			if(!s2)
				return 0;

			// resume at previous position+1
			w = w2;
			s = s2++;
		}
	}

	// strip trailing * in wildcard string
	while(*w == '*')
		w++;

	return (*w == '\0');
}
