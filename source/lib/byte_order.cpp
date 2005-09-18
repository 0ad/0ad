#include "precompiled.h"

#include "byte_order.h"
#include "sdl.h"


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


u64 read_le64(const void* p)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return SDL_Swap64(*(u64*)p);
#else
	return *(u64*)p;
#endif
}



u16 read_be16(const void* p)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return *(u16*)p;
#else
	const u8* _p = (const u8*)p;
	return (u16)_p[0] | (u16)_p[1] << 8;
#endif
}


u32 read_be32(const void* p)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return *(u32*)p;
#else
	return SDL_Swap32(*(u32*)p);
#endif
}


u64 read_be64(const void* p)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return *(u64*)p;
#else
	return SDL_Swap64(*(u64*)p);
#endif
}