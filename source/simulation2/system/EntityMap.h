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
#ifndef INCLUDED_ENTITYMAP
#define INCLUDED_ENTITYMAP

#include "Entity.h"

#include <utility>

static const size_t ENTITYMAP_DEFAULT_SIZE = 4096;
/**
 * A fast replacement for map<entity_id_t, T>.
 * Behaves like a much faster std:map.
 * The drawback is memory usage, as this uses sparse storage.
 * So creating/deleting entities will lead to inefficiency.
 */

template<class V, entity_id_t FIRST_VALID_ENTITY_ID = 1, size_t initialSize = ENTITYMAP_DEFAULT_SIZE>
class EntityMap
{
	static_assert(FIRST_VALID_ENTITY_ID > INVALID_ENTITY, "Entity IDs start at INVALID_ENTITY");
private:
	friend class TestEntityMap;
	
	EntityMap(const EntityMap&) = delete;
	EntityMap& operator=(const EntityMap&) = delete;

public:
	typedef entity_id_t key_type;
	typedef V mapped_type;
	typedef std::pair<entity_id_t, V> value_type;

private:
	std::vector<value_type> m_Data;

	// number of 'valid' entity id's
	size_t m_Count;

	// EntityMap keeps a valid entity ID at the end() so _iter<>::operator++ knows when to stop the loop
	// Use FIRST_VALID_ENTITY_ID since we are sure that that is indeed a valid ID.
#define ITERATOR_FENCE {FIRST_VALID_ENTITY_ID, V()}
public:

	EntityMap()
		: m_Count(0)
	{
		m_Data.reserve(initialSize);
		m_Data.push_back(ITERATOR_FENCE);
	}

	template<class U> struct _iter : public std::iterator<std::forward_iterator_tag, U>
	{
		U* val;
		_iter(U* init) : val(init) {}
		template<class T>
		_iter(const _iter<T>& it) : val(it.val) {}

		U& operator*() { return *val; }
		U* operator->() { return val; }

		_iter& operator++() // ++it
		{
			do
				++val;
			while (val->first == INVALID_ENTITY);
			return *this;
		}
		_iter operator++(int) // it++
		{
			_iter orig = *this;
			++(*this);
			return orig;
		}
		bool operator==(_iter other) { return val == other.val; }
		bool operator!=(_iter other) { return val != other.val; }
	};

	typedef _iter<value_type> iterator;
	typedef _iter<const value_type> const_iterator;

	iterator begin()
	{
		iterator it = &m_Data.front();
		if (it->first == INVALID_ENTITY)
			++it; // skip all invalid entities
		return it;
	}

	const_iterator begin() const
	{
		const_iterator it = &m_Data.front();
		if (it->first == INVALID_ENTITY)
			++it;
		return it;
	}

	iterator end()
	{
		return iterator(&m_Data.back()); // return the ITERATOR_FENCE, see above
	}

	const_iterator end() const
	{
		return const_iterator(&m_Data.back()); // return the ITERATOR_FENCE, see above
	}

	bool empty() const { return m_Count == 0; }
	size_t size() const { return m_Count; }

	std::pair<iterator, bool> insert_or_assign(const key_type& key, const mapped_type& value)
	{
		ENSURE(key >= FIRST_VALID_ENTITY_ID);

		if (key-FIRST_VALID_ENTITY_ID+1 >= m_Data.size())
		{
			// Fill [end(),…,key[ invalid entities, our new key, and then add a new iterator gate at the end;
			// resize, make sure to keep a valid entity ID at the end by adding a new ITERATOR_FENCE
			m_Data.back().first = INVALID_ENTITY; // reset current iterator gate
			m_Data.resize(key-FIRST_VALID_ENTITY_ID+2, {INVALID_ENTITY, V()});
			m_Data.back() = ITERATOR_FENCE;
		}
		bool inserted = false;
		if (m_Data[key-FIRST_VALID_ENTITY_ID].first == INVALID_ENTITY)
		{
			inserted = true;
			++m_Count;
		}

		m_Data[key-FIRST_VALID_ENTITY_ID] = {key, value};
		return {iterator(&m_Data[key-FIRST_VALID_ENTITY_ID]), inserted};
	}

	size_t erase(iterator it)
	{
		if (it->first != INVALID_ENTITY && it != end())
		{
			it->first = INVALID_ENTITY;
			it->second.~V(); // call dtor
			--m_Count;
			return 1;
		}
		return 0;
	}

	size_t erase(const key_type& key)
	{
		if (key-FIRST_VALID_ENTITY_ID+1 < m_Data.size())
			return erase(&m_Data.front() + key - FIRST_VALID_ENTITY_ID);
		return 0;
	}

	void clear()
	{
		m_Data.clear();
		m_Count = 0;
		m_Data.push_back(ITERATOR_FENCE);
	}

	iterator find(const key_type& key)
	{
		if (key-FIRST_VALID_ENTITY_ID+1 < m_Data.size() && m_Data[key-FIRST_VALID_ENTITY_ID].first != INVALID_ENTITY)
			return &m_Data.front() + (key - FIRST_VALID_ENTITY_ID);
		return end();
	}

	const_iterator find(const key_type& key) const
	{
		if (key-FIRST_VALID_ENTITY_ID+1 < m_Data.size() && m_Data[key-FIRST_VALID_ENTITY_ID].first != INVALID_ENTITY)
			return &m_Data.front() + (key - FIRST_VALID_ENTITY_ID);
		return end();
	}
#undef ITERATOR_FENCE
};

template<class VSerializer>
struct SerializeEntityMap
{
	template<class V>
	void operator()(ISerializer& serialize, const char* UNUSED(name), EntityMap<V>& value)
	{
		size_t len = value.size();
		serialize.NumberU32_Unbounded("length", (u32)len);
		size_t count = 0;
		for (typename EntityMap<V>::iterator it = value.begin(); it != value.end(); ++it)
		{
			serialize.NumberU32_Unbounded("key", it->first);
			VSerializer()(serialize, "value", it->second);
			count++;
		}
		// test to see if the entityMap count wasn't wrong
		// (which causes a crashing deserialisation)
		ENSURE(count == len);
	}

	template<class V>
	void operator()(IDeserializer& deserialize, const char* UNUSED(name), EntityMap<V>& value)
	{
		value.clear();
		uint32_t len;
		deserialize.NumberU32_Unbounded("length", len);
		for (size_t i = 0; i < len; ++i)
		{
			entity_id_t k;
			V v;
			deserialize.NumberU32_Unbounded("key", k);
			VSerializer()(deserialize, "value", v);
			value.insert_or_assign(k, v);
		}
	}
};


#endif
