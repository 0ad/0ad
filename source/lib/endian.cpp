#include "precompiled.h"

// don't need to implement on VC - header maps bswap* to instrinsics
#ifndef _MSC_VER

u16 bswap16(u16 x)
{
	return (u16)(((x & 0xff) << 8) | (x >> 8));
}


u32 bswap32(u32 x)
{
	u32 t = x;

	for(int i = 0; i < 4; i++)
	{
		x <<= 8;
		x |= t & 0xff;
	}

	return x;
}


u64 bswap64(u64 x)
{
	u64 t = x;

	for(int i = 0; i < 8; i++)
	{
		x <<= 8;
		x |= t & 0xff;
	}

	return x;
}


#endif	// #ifndef _MSC_VER