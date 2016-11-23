/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_CSTRINTERN
#define INCLUDED_CSTRINTERN

class CStrInternInternals;

/**
 * Interned 8-bit strings.
 * Each instance with the same string content is a pointer to the same piece of
 * memory, allowing very fast string comparisons.
 *
 * Since a CStrIntern is just a dumb pointer, copying is very fast,
 * and pass-by-value should be preferred over pass-by-reference.
 *
 * Memory allocated for strings will never be freed, so don't use this for
 * unbounded numbers of strings (e.g. text rendered by gameplay scripts) -
 * it's intended for a small number of short frequently-used strings.
 *
 * Not thread-safe - only allocate these strings from the main thread.
 */
class CStrIntern
{
public:
	CStrIntern();
	explicit CStrIntern(const char* str);
	explicit CStrIntern(const std::string& str);

	/**
	 * Returns cached FNV1-A hash of the string.
	 */
	u32 GetHash() const;

	/**
	 * Returns null-terminated string.
	 */
	const char* c_str() const;

	/**
	 * Returns length of string in bytes.
	 */
	size_t length() const;

	bool empty() const;

	/**
	 * Returns as std::string.
	 */
	const std::string& string() const;

	/**
	 * String equality.
	 */
	bool operator==(const CStrIntern& b) const
	{
		return m == b.m;
	}

	bool operator!=(const CStrIntern& b) const
	{
		return m != b.m;
	}

	/**
	 * Compare with some arbitrary total order.
	 * (In particular, this is not alphabetic order,
	 * and is not consistent between runs of the game.)
	 */
	bool operator<(const CStrIntern& b) const
	{
		return m < b.m;
	}

private:
	CStrInternInternals* m;
};

static inline size_t hash_value(const CStrIntern& str)
{
	return str.GetHash();
}

#define X(id) extern CStrIntern str_##id;
#define X2(id, str) extern CStrIntern str_##id;
#include "CStrInternStatic.h"
#undef X
#undef X2

#endif // INCLUDED_CSTRINTERN
