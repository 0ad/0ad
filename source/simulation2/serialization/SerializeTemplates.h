/* Copyright (C) 2010 Wildfire Games.
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

	template<typename K, typename V>
	void operator()(IDeserializer& deserialize, const char* UNUSED(name), std::map<K, V>& value)
	{
		value.clear();
		u32 len;
		deserialize.NumberU32_Unbounded("length", len);
		for (size_t i = 0; i < len; ++i)
		{
			K k;
			V v;
			KS()(deserialize, "key", k);
			VS()(deserialize, "value", v);
			value.insert(std::make_pair(k, v));
		}
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
		value = (T)val;
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

struct SerializeWaypoint
{
	void operator()(ISerializer& serialize, const char* UNUSED(name), const ICmpPathfinder::Waypoint& value)
	{
		serialize.NumberFixed_Unbounded("waypoint x", value.x);
		serialize.NumberFixed_Unbounded("waypoint z", value.z);
	}

	void operator()(IDeserializer& deserialize, const char* UNUSED(name), ICmpPathfinder::Waypoint& value)
	{
		deserialize.NumberFixed_Unbounded("waypoint x", value.x);
		deserialize.NumberFixed_Unbounded("waypoint z", value.z);
	}
};

struct SerializeGoal
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), ICmpPathfinder::Goal& value)
	{
		SerializeU8_Enum<ICmpPathfinder::Goal::Type, ICmpPathfinder::Goal::SQUARE>()(serialize, "type", value.type);
		serialize.NumberFixed_Unbounded("goal x", value.x);
		serialize.NumberFixed_Unbounded("goal z", value.z);
		serialize.NumberFixed_Unbounded("goal u x", value.u.X);
		serialize.NumberFixed_Unbounded("goal u z", value.u.Y);
		serialize.NumberFixed_Unbounded("goal v x", value.v.X);
		serialize.NumberFixed_Unbounded("goal v z", value.v.Y);
		serialize.NumberFixed_Unbounded("goal hw", value.hw);
		serialize.NumberFixed_Unbounded("goal hh", value.hh);
	}
};

#endif // INCLUDED_SERIALIZETEMPLATES
