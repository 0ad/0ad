/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Fowler/Noll/Vo string hash
 */

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
