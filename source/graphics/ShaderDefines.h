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

#include "graphics/ShaderProgramPtr.h"
#include "ps/CStr.h"
#include "ps/CStrIntern.h"

#include <boost/unordered_map.hpp>

class CVector4D;

/**
 * Represents a mapping of name strings to value, for use with
 * CShaderDefines (values are strings) and CShaderUniforms (values are vec4s).
 * 
 * Stored as interned vectors of name-value pairs, to support high performance
 * comparison operators.
 *
 * Not thread-safe - must only be used from the main thread.
 */
template<typename value_t>
class CShaderParams
{
public:
	/**
	 * Create an empty map of defines.
	 */
	CShaderParams();

	/**
	 * Add a name and associated value to the map of parameters.
	 * If the name is already defined, its value will be replaced.
	 */
	void Set(CStrIntern name, const value_t& value);

	/**
	 * Add all the names and values from another set of parameters.
	 * If any name is already defined in this object, its value will be replaced.
	 */
	void SetMany(const CShaderParams& params);

	/**
	 * Return a copy of the current name/value mapping.
	 */
	std::map<CStrIntern, value_t> GetMap() const;

	/**
	 * Return a hash of the current mapping.
	 */
	size_t GetHash() const;

	/**
	 * Compare with some arbitrary total order.
	 * The order may be different each time the application is run
	 * (it is based on interned memory addresses).
	 */
	bool operator<(const CShaderParams& b) const
	{
		return m_Items < b.m_Items;
	}

	/**
	 * Fast equality comparison.
	 */
	bool operator==(const CShaderParams& b) const
	{
		return m_Items == b.m_Items;
	}

	/**
	 * Fast inequality comparison.
	 */
	bool operator!=(const CShaderParams& b) const
	{
		return m_Items != b.m_Items;
	}

	struct SItems
	{
		// Name/value pair
		typedef std::pair<CStrIntern, value_t> Item;
		
		// Sorted by name; no duplicated names
		std::vector<Item> items;
		
		size_t hash;
		
		void RecalcHash();
	};

protected:
 	SItems* m_Items; // interned value

private:
	typedef boost::unordered_map<SItems, shared_ptr<SItems> > InternedItems_t;
	static InternedItems_t s_InternedItems;

	/**
	 * Returns a pointer to an SItems equal to @p items.
	 * The pointer will be valid forever, and the same pointer will be returned
	 * for any subsequent requests for an equal items list.
	 */
	static SItems* GetInterned(const SItems& items);

	CShaderParams(SItems* items);
	static CShaderParams CreateEmpty();
	static CShaderParams s_Empty;
};

/**
 * Represents a mapping of name strings to value strings, for use with
 * \#if and \#ifdef and similar conditionals in shaders.
 *
 * Not thread-safe - must only be used from the main thread.
 */
class CShaderDefines : public CShaderParams<CStrIntern>
{
public:
	/**
	 * Add a name and associated value to the map of defines.
	 * If the name is already defined, its value will be replaced.
	 */
	void Add(CStrIntern name, CStrIntern value);

	/**
	 * Return the value for the given name as an integer, or 0 if not defined.
	 */
	int GetInt(const char* name) const;
};

/**
 * Represents a mapping of name strings to value CVector4Ds, for use with
 * uniforms in shaders.
 *
 * Not thread-safe - must only be used from the main thread.
 */
class CShaderUniforms : public CShaderParams<CVector4D>
{
public:
	/**
	 * Add a name and associated value to the map of uniforms.
	 * If the name is already defined, its value will be replaced.
	 */
	void Add(const char* name, const CVector4D& value);

	/**
	 * Return the value for the given name, or (0,0,0,0) if not defined.
	 */
	CVector4D GetVector(const char* name) const;

	/**
	 * Bind the collection of uniforms onto the given shader.
	 */
	void BindUniforms(const CShaderProgramPtr& shader) const;
};

// Add here the types of queries we can make in the renderer
enum RENDER_QUERIES
{
	RQUERY_TIME,
	RQUERY_WATER_TEX,
	RQUERY_SKY_CUBE
};

/**
 * Uniform values that need to be evaluated in the renderer.
 * 
 * Not thread-safe - must only be used from the main thread.
 */
class CShaderRenderQueries
{
public:
	typedef std::pair<int, CStrIntern> RenderQuery;
	
	void Add(const char* name);
	size_t GetSize() const { return m_Items.size(); }
	RenderQuery GetItem(size_t i) const { return m_Items[i]; }
private:
	std::vector<RenderQuery> m_Items;
};


enum DEFINE_CONDITION_TYPES
{
	DCOND_DISTANCE
};

class CShaderConditionalDefines
{
public:
	struct CondDefine
	{
		CStrIntern m_DefName;
		CStrIntern m_DefValue;
		int m_CondType;
		std::vector<float> m_CondArgs;
	};
	
	void Add(const char* defname, const char* defvalue, int type, std::vector<float> &args);
	size_t GetSize() const { return m_Defines.size(); }
	const CondDefine& GetItem(size_t i) const { return m_Defines[i]; }
	
private:
	std::vector<CondDefine> m_Defines;
};

#endif // INCLUDED_SHADERDEFINES
