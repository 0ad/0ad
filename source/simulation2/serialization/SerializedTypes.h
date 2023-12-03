/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_SERIALIZED_TYPES
#define INCLUDED_SERIALIZED_TYPES

/**
 * @file
 * Provide specializations for some generic types and containers.
 */

#include "simulation2/serialization/SerializeTemplates.h"

#include "lib/types.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

template<typename T, size_t N>
struct SerializeHelper<std::array<T, N>>
{
	void operator()(ISerializer& serialize, const char* name, std::array<T, N>& value)
	{
		for (size_t i = 0; i < N; ++i)
			Serializer(serialize, name, value[i]);
	}

	void operator()(IDeserializer& deserialize, const char* name, std::array<T, N>& value)
	{
		for (size_t i = 0; i < N; ++i)
			Serializer(deserialize, name, value[i]);
	}
};

template<typename T>
struct SerializeHelper<std::vector<T>>
{
	void operator()(ISerializer& serialize, const char* name, std::vector<T>& value)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		for (size_t i = 0; i < len; ++i)
			Serializer(serialize, name, value[i]);
	}

	void operator()(IDeserializer& deserialize, const char* name, std::vector<T>& value)
	{
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		value.reserve(len); // TODO: watch out for out-of-memory
		for (size_t i = 0; i < len; ++i)
		{
			T el;
			Serializer(deserialize, name, el);
			value.emplace_back(el);
		}
	}
};

template<typename T>
struct SerializeHelper<std::set<T>>
{
	void operator()(ISerializer& serialize, const char* name, const std::set<T>& value)
	{
		serialize.NumberU32_Unbounded("size", static_cast<u32>(value.size()));
		for (const T& elem : value)
			Serializer(serialize, name, elem);
	}

	void operator()(IDeserializer& deserialize, const char* name, std::set<T>& value)
	{
		value.clear();
		u32 size;
		deserialize.NumberU32_Unbounded("size", size);
		for (size_t i = 0; i < size; ++i)
		{
			T el;
			Serializer(deserialize, name, el);
			value.emplace(std::move(el));
		}
	}
};

template<typename K, typename V>
struct SerializeHelper<std::map<K, V>>
{
	template<typename... Args>
	void operator()(ISerializer& serialize, const char* UNUSED(name), std::map<K, V>& value, Args&&... args)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		for (typename std::map<K, V>::iterator it = value.begin(); it != value.end(); ++it)
		{
			Serializer(serialize, "key", it->first, std::forward<Args>(args)...);
			Serializer(serialize, "value", it->second, std::forward<Args>(args)...);
		}
	}

	template<typename... Args>
	void operator()(IDeserializer& deserialize, const char* UNUSED(name), std::map<K, V>& value, Args&&... args)
	{
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		for (size_t i = 0; i < len; ++i)
		{
			K k;
			V v;
			Serializer(deserialize, "key", k, std::forward<Args>(args)...);
			Serializer(deserialize, "value", v, std::forward<Args>(args)...);
			value.emplace(std::move(k), std::move(v));
		}
	}
};

// We have to order the map before serializing to make things consistent
template<typename K, typename V>
struct SerializeHelper<std::unordered_map<K, V>>
{
	void operator()(ISerializer& serialize, const char* name, std::unordered_map<K, V>& value)
	{
		std::map<K, V> ordered_value(value.begin(), value.end());
		Serializer(serialize, name, ordered_value);
	}

	void operator()(IDeserializer& deserialize, const char* name, std::unordered_map<K, V>& value)
	{
		Serializer(deserialize, name, value);
	}
};

template<>
struct SerializeHelper<u8>
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


template<typename Enum>
struct SerializeHelper<Enum, std::enable_if_t<std::is_enum_v<Enum>>>
{
	void operator()(ISerializer& serialize, const char* name, Enum value, Enum&& max)
	{
		serialize.NumberU8(name, static_cast<u8>(value), 0, static_cast<u8>(max));
	}

	void operator()(IDeserializer& deserialize, const char* name, Enum& value, Enum&& max)
	{
		u8 val;
		deserialize.NumberU8(name, val, 0, static_cast<u8>(max));
		value = static_cast<Enum>(val);
	}
};


template<>
struct SerializeHelper<u16>
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

template<>
struct SerializeHelper<u32>
{
	template<typename... Args>
	void operator()(ISerializer& serialize, const char* name, u32 value, Args&&...)
	{
		serialize.NumberU32_Unbounded(name, value);
	}

	template<typename... Args>
	void operator()(IDeserializer& deserialize, const char* name, u32& value, Args&&...)
	{
		deserialize.NumberU32_Unbounded(name, value);
	}
};

template<>
struct SerializeHelper<i32>
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

template<>
struct SerializeHelper<bool>
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

template<typename StringLike>
struct SerializeHelper<StringLike, std::enable_if_t<std::is_base_of_v<std::string, StringLike>>>
{
	template<typename T>
	void operator()(ISerializer& serialize, const char* name, T&& value)
	{
		serialize.StringASCII(name, value, 0, UINT32_MAX);
	}

	template<typename T>
	void operator()(IDeserializer& deserialize, const char* name, T& value)
	{
		deserialize.StringASCII(name, value, 0, UINT32_MAX);
	}
};

#endif // INCLUDED_SERIALIZED_TYPES
