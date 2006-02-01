#include "precompiled.h"

#include "byte_order.h"
#include "sdl.h"


u16 to_le16(u16 x)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return SDL_Swap16(x);
#else
	return x;
#endif
}

u32 to_le32(u32 x)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return SDL_Swap32(x);
#else
	return x;
#endif
}

u64 to_le64(u64 x)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return SDL_Swap64(x);
#else
	return x;
#endif
}


u16 to_be16(u16 x)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return x;
#else
	return SDL_Swap16(x);
#endif
}

u32 to_be32(u32 x)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return x;
#else
	return SDL_Swap32(x);
#endif
}

u64 to_be64(u64 x)
{
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
	return x;
#else
	return SDL_Swap64(x);
#endif
}


u16 read_le16(const void* p)
{
	return to_le16(*(u16*)p);
}

u32 read_le32(const void* p)
{
	return to_le32(*(u32*)p);
}

u64 read_le64(const void* p)
{
	return to_le64(*(u64*)p);
}


u16 read_be16(const void* p)
{
	return to_be16(*(u16*)p);
}

u32 read_be32(const void* p)
{
	return to_be32(*(u32*)p);
}

u64 read_be64(const void* p)
{
	return to_be64(*(u64*)p);
}


void write_le16(void* p, u16 x)
{
	*(u16*)p = to_le16(x);
}

void write_le32(void* p, u32 x)
{
	*(u32*)p = to_le32(x);
}

void write_le64(void* p, u64 x)
{
	*(u64*)p = to_le64(x);
}


void write_be16(void* p, u16 x)
{
	*(u16*)p = to_be16(x);
}

void write_be32(void* p, u32 x)
{
	*(u32*)p = to_be32(x);
}

void write_be64(void* p, u64 x)
{
	*(u64*)p = to_be64(x);
}
