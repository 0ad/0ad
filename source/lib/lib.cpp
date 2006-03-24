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
#include "lib/app_hooks.h"

#include "sysdep/sysdep.h"

#ifndef SELF_TEST_ENABLED
#define SELF_TEST_ENABLED 0
#endif


// FNV1-A hash - good for strings.
// if len = 0 (default), treat buf as a C-string;
// otherwise, hash <len> bytes of buf.
u32 fnv_hash(const void* buf, size_t len)
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
u64 fnv_hash64(const void* buf, size_t len)
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
u32 fnv_lc_hash(const char* str, size_t len)
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





bool is_pow2(uint n)
{
	// 0 would pass the test below but isn't a POT.
	if(n == 0)
		return false;
	return (n & (n-1l)) == 0;
}


// return -1 if not an integral power of 2,
// otherwise the base2 logarithm

int ilog2(uint n)
{
	int bit_index;	// return value

#if CPU_IA32 && HAVE_MS_ASM

	__asm
	{
		mov		ecx, [n]
		or		eax, -1			// return value if not a POT
		test	ecx, ecx
		jz		not_pot
		lea		edx, [ecx-1]
		test	ecx, edx
		jnz		not_pot
		bsf		eax, ecx
	not_pot:
		mov		[bit_index], eax
	}

#else

	if(!is_pow2(n))
		return -1;

	bit_index = 0;
	// note: compare against n directly because it is known to be a POT.
	for(uint bit_value = 1; bit_value != n; bit_value *= 2)
		bit_index++;

#endif

	debug_assert(-1 <= bit_index && bit_index < (int)sizeof(int)*CHAR_BIT);
	debug_assert(bit_index == -1 || n == (1u << bit_index));
	return bit_index;
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


cassert(sizeof(int)*CHAR_BIT == 32);	// otherwise change round_up_to_pow2

uint round_up_to_pow2(uint x)
{
	// fold upper bit into lower bits; leaves same MSB set but
	// everything below it 1. adding 1 yields next POT.
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x+1;
}


int ilog2(const float x)
{
	const u32 i = *(u32*)&x;
	u32 biased_exp = (i >> 23) & 0xff;
	return (int)biased_exp - 127;
}



// multiple must be a power of two.
uintptr_t round_up(const uintptr_t n, const uintptr_t multiple)
{
	debug_assert(is_pow2((long)multiple));
	const uintptr_t result = (n + multiple-1) & ~(multiple-1);
	debug_assert(n <= result && result < n+multiple);
	return result;
}

// multiple must be a power of two.
uintptr_t round_down(const uintptr_t n, const uintptr_t multiple)
{
	debug_assert(is_pow2((long)multiple));
	const uintptr_t result = n & ~(multiple-1);
	debug_assert(result <= n && n < result+multiple);
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


// these avoid a common mistake in using >> (ANSI requires shift count be
// less than the bit width of the type).

u32 u64_hi(u64 x)
{
	return (u32)(x >> 32);
}

u32 u64_lo(u64 x)
{
	return (u32)(x & 0xFFFFFFFF);
}

u16 u32_hi(u32 x)
{
	return (u16)(x >> 16);
}

u16 u32_lo(u32 x)
{
	return (u16)(x & 0xFFFF);
}



u64 u64_from_u32(u32 hi, u32 lo)
{
	u64 x = (u64)hi;
	x <<= 32;
	x |= lo;
	return x;
}

u32 u32_from_u16(u16 hi, u16 lo)
{
	u32 x = (u32)hi;
	x <<= 16;
	x |= lo;
	return x;
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




// return random integer in [min, max).
// avoids several common pitfalls; see discussion at
// http://www.azillionmonkeys.com/qed/random.html

// Silly heuristic: glibc has RAND_MAX of 2^31 - 1 while MS CRT has 2^15-1 or something...
#if RAND_MAX < 65536
static const uint XRAND_MAX = (RAND_MAX+1)*(RAND_MAX+1) - 1;

uint fullrand()
{
	return rand()*(RAND_MAX+1) + rand();
}
#else
static const uint XRAND_MAX = RAND_MAX;

uint fullrand()
{
	return rand();
}
#endif

uint rand(uint min_inclusive, uint max_exclusive)
{
	const uint range = (max_exclusive-min_inclusive);
	// huge interval or min >= max
	if(range == 0 || range > XRAND_MAX)
	{
		WARN_ERR(ERR_INVALID_PARAM);
		return 0;
	}

	const uint inv_range = XRAND_MAX / range;

	// generate random number in [0, range)
	uint x;
	do
		x = fullrand();
	while(x >= range * inv_range);
	x /= inv_range;

	x += min_inclusive;
	debug_assert(x < max_exclusive);
	return x;
}


//-----------------------------------------------------------------------------
// built-in self test
//-----------------------------------------------------------------------------

#if SELF_TEST_ENABLED
namespace test {

static void test_log2()
{
	TEST(ilog2(0u) == -1);
	TEST(ilog2(3u) == -1);
	TEST(ilog2(0xffffffffu) == -1);
	TEST(ilog2(1u) == 0);
	TEST(ilog2(256u) == 8);
	TEST(ilog2(0x80000000u) == 31);
}

static void test_rand()
{
	// complain if huge interval or min > max
	TEST(rand(1, 0) == 0);
	TEST(rand(2, ~0u) == 0);

	// returned number must be in [min, max)
	for(int i = 0; i < 100; i++)
	{
		uint min = rand(), max = min+rand();
		uint x = rand(min, max);
		TEST(min <= x && x < max);
	}

	// make sure both possible values are hit
	uint ones = 0, twos = 0;
	for(int i = 0; i < 100; i++)
	{
		uint x = rand(1, 3);
		// paranoia: don't use array (x might not be 1 or 2 - checked below)
		if(x == 1) ones++; if(x == 2) twos++;
	}
	TEST(ones+twos == 100);
	TEST(ones > 10 && twos > 10);
}

static void self_test()
{
	test_log2();
	test_rand();
}

SELF_TEST_RUN;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED
