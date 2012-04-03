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

#include "precompiled.h"

#include "ShaderDefines.h"

#include "ps/ThreadUtil.h"

#include <boost/unordered_map.hpp>

size_t hash_value(const CShaderDefines::SItems& items)
{
	return items.hash;
}

bool operator==(const CShaderDefines::SItems& a, const CShaderDefines::SItems& b)
{
	return a.items == b.items;
}

struct ItemNameCmp
{
	typedef CShaderDefines::SItems::Item first_argument_type;
	typedef CShaderDefines::SItems::Item second_argument_type;
	bool operator()(const CShaderDefines::SItems::Item& a, const CShaderDefines::SItems::Item& b) const
	{
		return a.first < b.first;
	}
};

struct ItemNameGeq
{
	bool operator()(const CShaderDefines::SItems::Item& a, const CShaderDefines::SItems::Item& b) const
	{
		return !(b.first < a.first);
	}
};

typedef boost::unordered_map<CShaderDefines::SItems, shared_ptr<CShaderDefines::SItems> > InternedItems_t;
static InternedItems_t g_InternedItems;

CShaderDefines::SItems* CShaderDefines::GetInterned(const SItems& items)
{
	ENSURE(ThreadUtil::IsMainThread()); // g_InternedItems is not thread-safe

	InternedItems_t::iterator it = g_InternedItems.find(items);
	if (it != g_InternedItems.end())
		return it->second.get();

	// Sanity test: the items list is meant to be sorted by name.
	// This is a reasonable place to verify that, since this will be called once per distinct SItems.
	ENSURE(std::adjacent_find(items.items.begin(), items.items.end(), std::binary_negate<ItemNameCmp>(ItemNameCmp())) == items.items.end());

	shared_ptr<SItems> ptr(new SItems(items));
	g_InternedItems.insert(std::make_pair(items, ptr));
	return ptr.get();
}

CShaderDefines::CShaderDefines()
{
	SItems items;
	items.RecalcHash();
	m_Items = GetInterned(items);
}

void CShaderDefines::Add(const char* name, const char* value)
{
	SItems items = *m_Items;

	SItems::Item addedItem = std::make_pair(CStrIntern(name), CStrIntern(value));

	// Add the new item in a way that preserves the sortedness and uniqueness of item names
	for (std::vector<SItems::Item>::iterator it = items.items.begin(); ; ++it)
	{
		if (it == items.items.end() || addedItem.first < it->first)
		{
			items.items.insert(it, addedItem);
			break;
		}
		else if (addedItem.first == it->first)
		{
			it->second = addedItem.second;
			break;
		}
	}

	items.RecalcHash();
	m_Items = GetInterned(items);
}

void CShaderDefines::Add(const CShaderDefines& defines)
{
	SItems items;
	// set_union merges the two sorted lists into a new sorted list;
	// if two items are equivalent (i.e. equal names, possibly different values)
	// then the one from the first list is kept
	std::set_union(
		defines.m_Items->items.begin(), defines.m_Items->items.end(),
		m_Items->items.begin(), m_Items->items.end(),
		std::inserter(items.items, items.items.begin()),
		ItemNameCmp());
	items.RecalcHash();
	m_Items = GetInterned(items);
}

std::map<CStr, CStr> CShaderDefines::GetMap() const
{
	std::map<CStr, CStr> ret;
	for (size_t i = 0; i < m_Items->items.size(); ++i)
		ret[m_Items->items[i].first.string()] = m_Items->items[i].second.string();
	return ret;
}

int CShaderDefines::GetInt(const char* name) const
{
	CStrIntern nameIntern(name);
	for (size_t i = 0; i < m_Items->items.size(); ++i)
	{
		if (m_Items->items[i].first == nameIntern)
		{
			int ret;
			std::stringstream str(m_Items->items[i].second.c_str());
			str >> ret;
			return ret;
		}
	}
	return 0;
}

size_t CShaderDefines::GetHash() const
{
	return m_Items->hash;
}

void CShaderDefines::SItems::RecalcHash()
{
	size_t h = 0;
	for (size_t i = 0; i < items.size(); ++i)
	{
		boost::hash_combine(h, items[i].first.GetHash());
		boost::hash_combine(h, items[i].second.GetHash());
	}
	hash = h;
}
