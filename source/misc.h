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

#ifdef __cplusplus
extern "C" {
#endif


#define UNUSED(param) (void)param;

#define ONCE(code) { static bool done; if(!done) { code; }; done = true; }


const u32 KB = 1 << 10;
const u32 MB = 1 << 20;


#ifdef _WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif


#ifndef _MCW_PC
#define _MCW_PC		0x0300		// Precision Control
#endif
#ifndef _PC_24
#define	_PC_24		0x0000		// 24 bits
#endif

extern uint _control87(uint new_cw, uint mask);


extern u32 fnv_hash(const char* str, int len);

#define snprintf _snprintf
#define vsnprintf _vsnprintf

#define BIT(n) (1ul << (n))

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


extern u16 bswap16(u16);
extern u32 bswap32(u32);


static inline u16 read_le16(const void* p)
{
#ifdef BIG_ENDIAN
	const u8* _p = p;
	return (u16)_p[0] | (u16)_p[1] << 8;
#else
	return *(u16*)p;
#endif
}


static inline u32 read_le32(const void* p)
{
#ifdef BIG_ENDIAN
	u32 t = 0;
	for(int i = 0; i < 32; i += 8)
		t |= (*(const u8*)p++) << i;

	return t;
#else
	return *(u32*)p;
#endif
}


extern bool is_pow2(int n);


// return -1 if not an integral power of 2,
// otherwise the base2 logarithm
extern int log2(const int n);


extern long round_up(long val, int multiple);

extern double ceil(double f);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __MISC_H__
