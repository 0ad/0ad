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

#include "types.h"
#include "lib.h"

#include "sdl.h"	// endian functions

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// more powerful atexit, with 0 or 1 parameters.
// callable before libc initialized, frees up the real atexit table,
// and often obviates a separate cleanup_everything function.
//
// problem: some of the functions registered here must be called after
// all other shutdown code (e.g. Winsock cleanup).
// we can't wedge ourselves between the regular atexit calls and
// process termination, so hooking exit isn't possible.
// need to use regular atexit, which must be called after _cinit.
// AFAIK, we can't interpose ourselves between libc init and constructors
// either, so constructors MUST NOT:
// - exit() (otherwise, some resources leak, because our atexit handler
//   wouldn't have been registered yet - it's done from main())
// - call atexit (our exit handler would be called before its handler,
//   so we may have shut down something important already).

static const int MAX_EXIT_FUNCS = 32;


static struct ExitFunc
{
	void* func;
	uintptr_t arg;
	CallConvention cc;
}
exit_funcs[MAX_EXIT_FUNCS];
static int num_exit_funcs;





// call all registered exit handlers in LIFO order.
// called from exit, so don't worry about thread safety.
static void call_exit_funcs(void)
{
	ExitFunc* p = exit_funcs;
	for(int i = num_exit_funcs-1; i >= 0; i--)
	{
		switch(p->cc)
		{
		case CC_CDECL_0:
			((void(*)(void))p->func)();
			break;
		case CC_CDECL_1:
			((void(*)(uintptr_t))p->func)(p->arg);
			break;
#ifdef _WIN32
		case CC_STDCALL_0:
			((void(__stdcall*)(void))p->func)();
			break;
		case CC_STDCALL_1:
			((void(__stdcall*)(uintptr_t))p->func)(p->arg);
			break;
#endif
		default:
			debug_warn("call_exit_funcs: invalid calling convention in ExitFunc!");
		}
		p++;
	}
	num_exit_funcs = 0;
}


int atexit2(void* func, uintptr_t arg, CallConvention cc)
{
	if(num_exit_funcs >= MAX_EXIT_FUNCS)
	{
		debug_warn("atexit2: too many functions registered. increase MAX_EXIT_FUNCS");
		return -1;
	}
	ExitFunc* p = &exit_funcs[num_exit_funcs++];
	p->func = func;
	p->arg = arg;
	p->cc = cc;
	return 0;
}


int atexit2(void* func)
{
	return atexit2(func, 0, CC_CDECL_0);
}



// call from main as early as possible.
void lib_init()
{
	atexit(call_exit_funcs);
}










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


uintptr_t round_up(const uintptr_t n, const uintptr_t multiple)
{
	assert(multiple != 0);
	const uintptr_t padded = n + multiple-1;
	const uintptr_t remainder = padded % multiple;
	const uintptr_t result = padded - remainder;
	assert(n <= result && result < n+multiple);
	return result;
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


// input in [0, 1); convert to u8 range
u8 fp_to_u8(double in)
{
	if(!(0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 255;
	}

	int l = (int)(in * 255.0);
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

	long l = (long)(in * 65535.0);
	assert((unsigned long)l <= 65535);
	return (u16)l;
}





u16 read_le16(const void* p)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	const u8* _p = (const u8*)p;
	return (u16)_p[0] | (u16)_p[1] << 8;
#else
	return *(u16*)p;
#endif
}


u32 read_le32(const void* p)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return SDL_Swap32(*(u32*)p);
#else
	return *(u32*)p;
#endif
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
