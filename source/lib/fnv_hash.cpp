/**
 * =========================================================================
 * File        : fnv_hash.h
 * Project     : 0 A.D.
 * Description : Fowler/Noll/Vo string hash
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"


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
