/* Copyright (C) 2013 Wildfire Games.
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

/**
 * A fast replacement for map<entity_id_t, T>.
 * We make the following assumptions:
 *  - entity id's (keys) are unique
 *  - modifications (add / delete) are far less frequent then look-ups
 *  - preformance for iteration is important
 */
template<class T> class EntityMap
{
private:
	EntityMap(const EntityMap&);			// non-copyable
	EntityMap& operator=(const EntityMap&); // non-copyable

public:
	typedef entity_id_t key_type;
	typedef T mapped_type;
	template<class K, class V> struct key_val {
		typedef K first_type;
		typedef V second_type;
		K first;
		V second;
	};
	typedef key_val<entity_id_t, T> value_type;

private:
	size_t m_BufferSize;		// number of elements in the buffer
	size_t m_BufferCapacity;	// capacity of the buffer
	value_type* m_Buffer;		// vector with all the mapped key-value pairs

	size_t m_Count;				// number of 'valid' entity id's

public:

	inline EntityMap() : m_BufferSize(1), m_BufferCapacity(4096), m_Count(0)
	{
		// for entitymap we allocate the buffer right away
		// with first element in buffer being the Invalid Entity
		m_Buffer = (value_type*)malloc(sizeof(value_type) * (m_BufferCapacity + 1));

		// create the first element:
		m_Buffer[0].first = INVALID_ENTITY;
		m_Buffer[1].first = 0xFFFFFFFF; // ensure end() always has 0xFFFFFFFF
	}
	inline ~EntityMap()
	{
		free(m_Buffer);
	}

	// Iterators
	template<class U> struct _iter : public std::iterator<std::forward_iterator_tag, U>
	{
		U* val;
		inline _iter(U* init) : val(init) {}
		inline U& operator*() { return *val; }
		inline U* operator->() { return val; }
		inline _iter& operator++() // ++it
		{
			++val;
			while (val->first == INVALID_ENTITY) ++val; // skip any invalid entities
			return *this;
		}
		inline _iter& operator++(int) // it++
		{
			U* ptr = val;
			++val;
			while (val->first == INVALID_ENTITY) ++val; // skip any invalid entities
			return ptr;
		}
		inline bool operator==(_iter other) { return val == other.val; }
		inline bool operator!=(_iter other) { return val != other.val; }
		inline operator _iter<U const>() const { return _iter<U const>(val); }
	};
	
	typedef _iter<value_type> iterator;
	typedef _iter<value_type const> const_iterator;

	inline iterator begin()
	{
		value_type* ptr = m_Buffer + 1; // skip the first INVALID_ENTITY
		while (ptr->first == INVALID_ENTITY) ++ptr; // skip any other invalid entities
		return ptr;
	}
	inline iterator end()
	{
		return iterator(m_Buffer + m_BufferSize);
	}
	inline const_iterator begin() const
	{
		value_type* ptr = m_Buffer + 1; // skip the first INVALID_ENTITY
		while (ptr->first == INVALID_ENTITY) ++ptr; // skip any other invalid entities
		return ptr;
	}
	inline const_iterator end() const
	{
		return const_iterator(m_Buffer + m_BufferSize);
	}

	// Size
	inline bool empty() const { return m_Count == 0; }
	inline size_t size() const { return m_Count; }

	// Modification
	void insert(const key_type key, const mapped_type& value)
	{
		if (key >= m_BufferCapacity) // do we need to resize buffer?
		{
			size_t newCapacity = m_BufferCapacity + 4096;
			while (key >= newCapacity) newCapacity += 4096;
			// always allocate +1 behind the scenes, because end() must have a 0xFFFFFFFF key
			value_type* mem = (value_type*)realloc(m_Buffer, sizeof(value_type) * (newCapacity + 1));
			if (!mem) 
			{
				debug_warn("EntityMap::insert() realloc failed! Out of memory.");
				throw std::bad_alloc(); // fail to expand and insert
			}
			m_BufferCapacity = newCapacity;
			m_Buffer = mem;
			goto fill_gaps;
		}
		else if (key > m_BufferSize) // weird insert far beyond the end
		{
fill_gaps:
			// set all entity id's to INVALID_ENTITY inside the new range
			for (size_t i = m_BufferSize; i <= key; ++i)
				m_Buffer[i].first = INVALID_ENTITY;
			m_BufferSize = key; // extend the new size
		}

		value_type& item = m_Buffer[key];
		key_type oldKey = item.first;
		item.first = key;
		if (key == m_BufferSize) // push_back
		{
			++m_BufferSize; // expand
			++m_Count;
			new (&item.second) mapped_type(value); // copy ctor to init
			m_Buffer[m_BufferSize].first = 0xFFFFFFFF; // ensure end() always has 0xFFFFFFFF
		}
		else if(!item.first) // insert new to middle
		{
			++m_Count;
			new (&item.second) mapped_type(value); // copy ctor to init
		}
		else // set existing value
		{
			if (oldKey == INVALID_ENTITY)
				m_Count++;
			item.second = value; // overwrite existing
		}
	}
	
	void erase(iterator it)
	{
		value_type* ptr = it.val;
		if (ptr->first != INVALID_ENTITY)
		{
			ptr->first = INVALID_ENTITY;
			ptr->second.~T(); // call dtor
			--m_Count;
		}
	}
	void erase(const entity_id_t key)
	{
		if (key < m_BufferSize)
		{
			value_type* ptr = m_Buffer + key;
			if (ptr->first != INVALID_ENTITY)
			{
				ptr->first = INVALID_ENTITY;
				ptr->second.~T(); // call dtor
				--m_Count;
			}
		}
	}
	inline void clear()
	{
		// orphan whole range
		value_type* ptr = m_Buffer;
		value_type* end = m_Buffer + m_BufferSize;
		for (; ptr != end; ++ptr)
		{
			if (ptr->first != INVALID_ENTITY)
			{
				ptr->first = INVALID_ENTITY;
				ptr->second.~T(); // call dtor
			}
		}
		m_Count = 0; // no more valid entities
	}

	// Operations
	inline iterator find(const entity_id_t key)
	{
		if (key < m_BufferSize) // is this key in the range of existing entitites?
		{
			value_type* ptr = m_Buffer + key;
			if (ptr->first != INVALID_ENTITY)
				return ptr;
		}
		return m_Buffer + m_BufferSize; // return iterator end()
	}
	inline const_iterator find(const entity_id_t key) const
	{
		if (key < m_BufferSize) // is this key in the range of existing entitites?
		{
			const value_type* ptr = m_Buffer + key;
			if (ptr->first != INVALID_ENTITY)
				return ptr;
		}
		return m_Buffer + m_BufferSize; // return iterator end()
	}
	inline size_t count(const entity_id_t key) const
	{
		if (key < m_BufferSize)
		{
			if (m_Buffer[key].first != INVALID_ENTITY)
				return 1;
		}
		return 0;
	}
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
			value.insert(k, v);
		}
	}
};


#endif
