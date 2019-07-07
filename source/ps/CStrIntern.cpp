/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "CStrIntern.h"

#include "lib/fnv_hash.h"
#include "ps/CLogger.h"
#include "ps/ThreadUtil.h"

#include <boost/unordered_map.hpp>

class CStrInternInternals
{
public:
	CStrInternInternals(const char* str, size_t len)
		: data(str, str+len), hash(fnv_hash(str, len))
	{
// 		LOGWARNING("New interned string '%s'", data.c_str());
	}

	bool operator==(const CStrInternInternals& b) const
	{
		// Compare hash first for quick rejection of inequal strings
		return (hash == b.hash && data == b.data);
	}

	const std::string data;
	const u32 hash; // fnv_hash of data

private:
	CStrInternInternals& operator=(const CStrInternInternals&);
};

// Interned strings are stored in a hash table, indexed by string:

typedef std::string StringsKey;

struct StringsKeyHash
{
	size_t operator()(const StringsKey& key) const
	{
		return fnv_hash(key.c_str(), key.length());
	}
};

// To avoid std::string memory allocations when GetString does lookups in the
// hash table of interned strings, we make use of boost::unordered_map's ability
// to do lookups with a functionally equivalent proxy object:

struct StringsKeyProxy
{
	const char* str;
	size_t len;
};

struct StringsKeyProxyHash
{
	size_t operator()(const StringsKeyProxy& key) const
	{
		return fnv_hash(key.str, key.len);
	}
};

struct StringsKeyProxyEq
{
	bool operator()(const StringsKeyProxy& proxy, const StringsKey& key) const
	{
		return (proxy.len == key.length() && memcmp(proxy.str, key.c_str(), proxy.len) == 0);
	}
};

static boost::unordered_map<StringsKey, shared_ptr<CStrInternInternals>, StringsKeyHash> g_Strings;

#define X(id) CStrIntern str_##id(#id);
#define X2(id, str) CStrIntern str_##id(str);
#include "CStrInternStatic.h"
#undef X
#undef X2

static CStrInternInternals* GetString(const char* str, size_t len)
{
	// g_Strings is not thread-safe, so complain if anyone is using this
	// type in non-main threads. (If that's desired, g_Strings should be changed
	// to be thread-safe, preferably without sacrificing performance.)
	ENSURE(ThreadUtil::IsMainThread());

#if BOOST_VERSION >= 104200
	StringsKeyProxy proxy = { str, len };
	boost::unordered_map<StringsKey, shared_ptr<CStrInternInternals> >::iterator it =
		g_Strings.find(proxy, StringsKeyProxyHash(), StringsKeyProxyEq());
#else
	// Boost <= 1.41 doesn't support the new find(), so do a slightly less efficient lookup
	boost::unordered_map<StringsKey, shared_ptr<CStrInternInternals> >::iterator it =
		g_Strings.find(str);
#endif

	if (it != g_Strings.end())
		return it->second.get();

	shared_ptr<CStrInternInternals> internals(new CStrInternInternals(str, len));
	g_Strings.insert(std::make_pair(internals->data, internals));
	return internals.get();
}

CStrIntern::CStrIntern()
{
	*this = str__emptystring;
}

CStrIntern::CStrIntern(const char* str)
{
	m = GetString(str, strlen(str));
}

CStrIntern::CStrIntern(const std::string& str)
{
	m = GetString(str.c_str(), str.length());
}

u32 CStrIntern::GetHash() const
{
	return m->hash;
}

const char* CStrIntern::c_str() const
{
	return m->data.c_str();
}

size_t CStrIntern::length() const
{
	return m->data.length();
}

bool CStrIntern::empty() const
{
	return m->data.empty();
}

const std::string& CStrIntern::string() const
{
	return m->data;
}
