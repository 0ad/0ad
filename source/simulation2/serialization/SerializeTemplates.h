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

#ifndef INCLUDED_SERIALIZETEMPLATES
#define INCLUDED_SERIALIZETEMPLATES

/**
 * @file
 * Helper templates for serializing/deserializing common objects.
 */

#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/serialization/IDeserializer.h"
#include "simulation2/serialization/ISerializer.h"

#include <unordered_map>
#include <utility>

template<typename ELEM>
struct SerializeVector
{
	template<typename T>
	void operator()(ISerializer& serialize, const char* name, std::vector<T>& value)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		for (size_t i = 0; i < len; ++i)
			ELEM()(serialize, name, value[i]);
	}

	template<typename T>
	void operator()(IDeserializer& deserialize, const char* name, std::vector<T>& value)
	{
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		value.reserve(len); // TODO: watch out for out-of-memory
		for (size_t i = 0; i < len; ++i)
		{
			T el;
			ELEM()(deserialize, name, el);
			value.push_back(el);
		}
	}
};

template<typename ELEM>
struct SerializeRepetitiveVector
{
	template<typename T>
	void operator()(ISerializer& serialize, const char* name, std::vector<T>& value)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		if (len == 0)
			return;
		u32 count = 1;
		T prevVal = value[0];
		for (size_t i = 1; i < len; ++i)
		{
			if (prevVal == value[i])
			{
				count++;
				continue;
			}
			serialize.NumberU32_Unbounded("#", count);
			ELEM()(serialize, name, prevVal);
			count = 1;
			prevVal = value[i];
		}
		serialize.NumberU32_Unbounded("#", count);
		ELEM()(serialize, name, prevVal);
	}

	template<typename T>
	void operator()(IDeserializer& deserialize, const char* name, std::vector<T>& value)
	{
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		value.reserve(len); // TODO: watch out for out-of-memory
		for (size_t i = 0; i < len;)
		{
			u32 count;
			deserialize.NumberU32_Unbounded("#", count);
			T el;
			ELEM()(deserialize, name, el);
			i += count;
			value.insert(value.end(), count, el);
		}
	}
};

template<typename ELEM>
struct SerializeSet
{
	template<typename T>
	void operator()(ISerializer& serialize, const char* name, const std::set<T>& value)
	{
		serialize.NumberU32_Unbounded("size", static_cast<u32>(value.size()));
		for (const T& elem : value)
			ELEM()(serialize, name, elem);
	}

	template<typename T>
	void operator()(IDeserializer& deserialize, const char* name, std::set<T>& value)
	{
		value.clear();
		u32 size;
		deserialize.NumberU32_Unbounded("size", size);
		for (size_t i = 0; i < size; ++i)
		{
			T el;
			ELEM()(deserialize, name, el);
			value.emplace(std::move(el));
		}
	}
};

template<typename KS, typename VS>
struct SerializeMap
{
	template<typename K, typename V>
	void operator()(ISerializer& serialize, const char* UNUSED(name), std::map<K, V>& value)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		for (typename std::map<K, V>::iterator it = value.begin(); it != value.end(); ++it)
		{
			KS()(serialize, "key", it->first);
			VS()(serialize, "value", it->second);
		}
	}

	template<typename K, typename V, typename C>
	void operator()(ISerializer& serialize, const char* UNUSED(name), std::map<K, V>& value, C& context)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		for (typename std::map<K, V>::iterator it = value.begin(); it != value.end(); ++it)
		{
			KS()(serialize, "key", it->first);
			VS()(serialize, "value", it->second, context);
		}
	}

	template<typename M>
	void operator()(IDeserializer& deserialize, const char* UNUSED(name), M& value)
	{
		typedef typename M::key_type K;
		typedef typename M::value_type::second_type V; // M::data_type gives errors with gcc
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		for (size_t i = 0; i < len; ++i)
		{
			K k;
			V v;
			KS()(deserialize, "key", k);
			VS()(deserialize, "value", v);
			value.emplace(std::move(k), std::move(v));
		}
	}

	template<typename M, typename C>
	void operator()(IDeserializer& deserialize, const char* UNUSED(name), M& value, C& context)
	{
		typedef typename M::key_type K;
		typedef typename M::value_type::second_type V; // M::data_type gives errors with gcc
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		for (size_t i = 0; i < len; ++i)
		{
			K k;
			V v;
			KS()(deserialize, "key", k);
			VS()(deserialize, "value", v, context);
			value.emplace(std::move(k), std::move(v));
		}
	}
};

// We have to order the map before serializing to make things consistent
template<typename KS, typename VS>
struct SerializeUnorderedMap
{
	template<typename K, typename V>
	void operator()(ISerializer& serialize, const char* name, std::unordered_map<K, V>& value)
	{
		std::map<K, V> ordered_value(value.begin(), value.end());
		SerializeMap<KS, VS>()(serialize, name, ordered_value);
	}

	template<typename K, typename V>
	void operator()(IDeserializer& deserialize, const char* name, std::unordered_map<K, V>& value)
	{
		SerializeMap<KS, VS>()(deserialize, name, value);
	}
};

template<typename T, T max>
struct SerializeU8_Enum
{
	void operator()(ISerializer& serialize, const char* name, T value)
	{
		serialize.NumberU8(name, value, 0, max);
	}

	void operator()(IDeserializer& deserialize, const char* name, T& value)
	{
		u8 val;
		deserialize.NumberU8(name, val, 0, max);
		value = static_cast<T>(val);
	}
};

struct SerializeU8_Unbounded
{
	void operator()(ISerializer& serialize, const char* name, u8 value)
	{
		serialize.NumberU8_Unbounded(name, value);
	}

	void operator()(IDeserializer& deserialize, const char* name, u8& value)
	{
		deserialize.NumberU8_Unbounded(name, value);
	}
};

struct SerializeU16_Unbounded
{
	void operator()(ISerializer& serialize, const char* name, u16 value)
	{
		serialize.NumberU16_Unbounded(name, value);
	}

	void operator()(IDeserializer& deserialize, const char* name, u16& value)
	{
		deserialize.NumberU16_Unbounded(name, value);
	}
};

struct SerializeU32_Unbounded
{
	void operator()(ISerializer& serialize, const char* name, u32 value)
	{
		serialize.NumberU32_Unbounded(name, value);
	}

	void operator()(IDeserializer& deserialize, const char* name, u32& value)
	{
		deserialize.NumberU32_Unbounded(name, value);
	}
};

struct SerializeI32_Unbounded
{
	void operator()(ISerializer& serialize, const char* name, i32 value)
	{
		serialize.NumberI32_Unbounded(name, value);
	}

	void operator()(IDeserializer& deserialize, const char* name, i32& value)
	{
		deserialize.NumberI32_Unbounded(name, value);
	}
};

struct SerializeBool
{
	void operator()(ISerializer& serialize, const char* name, bool value)
	{
		serialize.Bool(name, value);
	}

	void operator()(IDeserializer& deserialize, const char* name, bool& value)
	{
		deserialize.Bool(name, value);
	}
};

struct SerializeString
{
	void operator()(ISerializer& serialize, const char* name, const std::string& value)
	{
		serialize.StringASCII(name, value, 0, UINT32_MAX);
	}

	void operator()(IDeserializer& deserialize, const char* name, std::string& value)
	{
		deserialize.StringASCII(name, value, 0, UINT32_MAX);
	}
};

struct SerializeWaypoint
{
	void operator()(ISerializer& serialize, const char* UNUSED(name), const Waypoint& value)
	{
		serialize.NumberFixed_Unbounded("waypoint x", value.x);
		serialize.NumberFixed_Unbounded("waypoint z", value.z);
	}

	void operator()(IDeserializer& deserialize, const char* UNUSED(name), Waypoint& value)
	{
		deserialize.NumberFixed_Unbounded("waypoint x", value.x);
		deserialize.NumberFixed_Unbounded("waypoint z", value.z);
	}
};

struct SerializeGoal
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), PathGoal& value)
	{
		SerializeU8_Enum<PathGoal::Type, PathGoal::INVERTED_SQUARE>()(serialize, "type", value.type);
		serialize.NumberFixed_Unbounded("goal x", value.x);
		serialize.NumberFixed_Unbounded("goal z", value.z);
		serialize.NumberFixed_Unbounded("goal u x", value.u.X);
		serialize.NumberFixed_Unbounded("goal u z", value.u.Y);
		serialize.NumberFixed_Unbounded("goal v x", value.v.X);
		serialize.NumberFixed_Unbounded("goal v z", value.v.Y);
		serialize.NumberFixed_Unbounded("goal hw", value.hw);
		serialize.NumberFixed_Unbounded("goal hh", value.hh);
		serialize.NumberFixed_Unbounded("maxdist", value.maxdist);
	}
};

#endif // INCLUDED_SERIALIZETEMPLATES
