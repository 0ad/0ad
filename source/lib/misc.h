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

#ifndef __MISC_H__
#define __MISC_H__

#include "types.h"
#include "config.h"

// bswap32 is overloaded!
#ifdef __cplusplus
//extern "C" {
#endif


#ifndef min
inline int min(int a, int b)
{
	return (a < b)? a : b;
}

inline int max(int a, int b)
{
	return (a > b)? a : b;
}
#endif

extern u16 addusw(u16 x, u16 y);
extern u16 subusw(u16 x, u16 y);



extern u16 bswap16(u16);
extern u32 bswap32(u32);

extern void bswap32(const u8* data, int cnt);

static inline u16 read_le16(const void* p)
{
#ifdef BIG_ENDIAN
	const u8* _p = (const u8*)p;
	return (u16)_p[0] | (u16)_p[1] << 8;
#else
	return *(u16*)p;
#endif
}


static inline u32 read_le32(const void* p)
{
#ifdef BIG_ENDIAN
	u32 t = 0;
	for(int i = 0; i < 4; i++)
	{
		t <<= 8;
		t |= *((const u8*)p)++;
	}
	return t;
#else
	return *(u32*)p;
#endif
}


extern bool is_pow2(long n);


// return -1 if not an integral power of 2,
// otherwise the base2 logarithm
extern int ilog2(const int n);


extern uintptr_t round_up(uintptr_t val, uintptr_t multiple);


// provide fminf for non-C99 compilers
#ifndef HAVE_C99
extern float fminf(float, float);
#endif




// big endian!
extern void base32(const int len, const u8* in, u8* out);

#ifndef _WIN32

char *_itoa(int, char *, int radix);
char *_ultoa(unsigned long int, char*, int radix);
char *_ltoa(long, char *, int radix);

#endif









#ifdef __cplusplus
//}
#endif

#endif	// #ifndef __MISC_H__
