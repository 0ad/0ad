#include "precompiled.h"

#include "lib.h"

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