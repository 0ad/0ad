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

#ifndef INCLUDED_SHADERDEFINES
#define INCLUDED_SHADERDEFINES

#include "ps/CStr.h"
#include "ps/CStrIntern.h"

/**
 * Represents a mapping of name to value strings, for use with
 * \#if and \#ifdef and similar conditionals in shaders.
 * 
 * Stored as interned vectors of string-pairs, to support high performance
 * comparison operators.
 *
 * Not thread-safe - must only be used from the main thread.
 */
class CShaderDefines
{
public:
	/**
	 * Create an empty map of defines.
	 */
	CShaderDefines();

	/**
	 * Add a name and associated value to the map of defines.
	 * If the name is already defined, its value will be replaced.
	 */
	void Add(const char* name, const char* value);
	
	/**
	 * Add all the names and values from another set of defines.
	 * If any name is already defined in this object, its value will be replaced.
	 */
	void Add(const CShaderDefines& defines);

	/**
	 * Return the value for the given name as an integer, or 0 if not defined.
	 */
	int GetInt(const char* name) const;

	/**
	 * Return a copy of the current name/value mapping.
	 */
	std::map<CStr, CStr> GetMap() const;

	/**
	 * Return a hash of the current mapping.
	 */
	size_t GetHash() const;

	/**
	 * Compare with some arbitrary total order.
	 * The order may be different each time the application is run
	 * (it is based on interned memory addresses).
	 */
	bool operator<(const CShaderDefines& b) const
	{
		return m_Items < b.m_Items;
	}

	/**
	 * Equality comparison.
	 */
	bool operator==(const CShaderDefines& b) const
	{
		return m_Items == b.m_Items;
	}

	struct SItems
	{
		// Name/value pair
		typedef std::pair<CStrIntern, CStrIntern> Item;
		
		// Sorted by name; no duplicated names
		std::vector<Item> items;
		
		size_t hash;
		
		void RecalcHash();
	};

private:
 	SItems* m_Items; // interned value

	/**
	 * Returns a pointer to an SItems equal to @p items.
	 * The pointer will be valid forever, and the same pointer will be returned
	 * for any subsequent requests for an equal items list.
	 */
	static SItems* GetInterned(const SItems& items);
};

#endif // INCLUDED_SHADERDEFINES
