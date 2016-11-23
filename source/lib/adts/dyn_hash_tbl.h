/* Copyright (c) 2011 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * dynamic (grow-able) hash table
 */

#ifndef INCLUDED_ADTS_DYN_HASH_TBL
#define INCLUDED_ADTS_DYN_HASH_TBL

#include <string.h>	// strcmp

#include "lib/fnv_hash.h"

template<typename Key, typename T> class DHT_Traits
{
public:
	static const size_t initial_entries = 16;
	size_t hash(Key key) const;
	bool equal(Key k1, Key k2) const;
	Key get_key(T t) const;
};

template<> class DHT_Traits<const char*, const char*>
{
public:
	static const size_t initial_entries = 512;
	size_t hash(const char* key) const
	{
		return (size_t)fnv_lc_hash(key);
	}
	bool equal(const char* k1, const char* k2) const
	{
		return !strcmp(k1, k2);
	}
	const char* get_key(const char* t) const
	{
		return t;
	}
};


// intended for pointer types
template<typename Key, typename T, typename Traits=DHT_Traits<Key,T> >
class DynHashTbl
{
	T* tbl;
	u16 num_entries;
	u16 max_entries;	// when initialized, = 2**n for faster modulo
	Traits tr;

	T& get_slot(Key key) const
	{
		size_t hash = tr.hash(key);
		ENSURE(max_entries != 0);	// otherwise, mask will be incorrect
		const size_t mask = max_entries-1;
		for(;;)
		{
			T& t = tbl[hash & mask];
			// empty slot encountered => not found
			if(!t)
				return t;
			// keys are actually equal => found it
			if(tr.equal(key, tr.get_key(t)))
				return t;
			// keep going (linear probing)
			hash++;
		}
	}

	void expand_tbl()
	{
		// alloc a new table (but don't assign it to <tbl> unless successful)
		T* old_tbl = tbl;
		tbl = (T*)calloc(max_entries*2, sizeof(T));
		if(!tbl)
		{
			tbl = old_tbl;
			throw std::bad_alloc();
		}

		max_entries *= 2;
		// must be set before get_slot

		// newly initialized, nothing to copy - done
		if(!old_tbl)
			return;

		// re-hash from old table into the new one
		for(size_t i = 0; i < max_entries/2u; i++)
		{
			T t = old_tbl[i];
			if(t)
				get_slot(tr.get_key(t)) = t;
		}
		free(old_tbl);
	}


public:

	DynHashTbl()
	{
		tbl = 0;
		clear();
	}

	~DynHashTbl()
	{
		free(tbl);
	}

	void clear()
	{
		// must remain usable after calling clear, so shrink the table to
		// its initial size but don't deallocate it completely
		SAFE_FREE(tbl);
		num_entries = 0;
		// rationale: must not set to 0 because expand_tbl only doubles the size.
		// don't keep the previous size when clearing because it may have become
		// huge and there is no provision for shrinking.
		max_entries = tr.initial_entries/2;	// will be doubled in expand_tbl
		expand_tbl();
	}

	void insert(const Key key, const T t)
	{
		// more than 75% full - increase table size.
		// do so before determining slot; this will invalidate previous pnodes.
		if(num_entries*4 >= max_entries*3)
			expand_tbl();

		T& slot = get_slot(key);
		ENSURE(slot == 0);	// not already present
		slot = t;
		num_entries++;
	}

	T find(Key key) const
	{
		return get_slot(key);
	}

	size_t size() const
	{
		return num_entries;
	}


	class iterator
	{
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef T value_type;
		typedef ptrdiff_t difference_type;
		typedef const T* pointer;
		typedef const T& reference;

		iterator()
		{
		}
		iterator(T* pos_, T* end_) : pos(pos_), end(end_)
		{
		}
		T& operator*() const
		{
			return *pos;
		}
		iterator& operator++()	// pre
		{
			do
			pos++;
			while(pos != end && *pos == 0);
			return (*this);
		}
		bool operator==(const iterator& rhs) const
		{
			return pos == rhs.pos;
		}
		bool operator<(const iterator& rhs) const
		{
			return (pos < rhs.pos);
		}

		// derived
		const T* operator->() const
		{
			return &**this;
		}
		bool operator!=(const iterator& rhs) const
		{
			return !(*this == rhs);
		}
		iterator operator++(int)	// post
		{
			iterator tmp =  *this; ++*this; return tmp;
		}

	protected:
		T* pos;
		T* end;
		// only used when incrementing (avoid going beyond end of table)
	};

	iterator begin() const
	{
		T* pos = tbl;
		while(pos != tbl+max_entries && *pos == 0)
			pos++;
		return iterator(pos, tbl+max_entries);
	}
	iterator end() const
	{
		return iterator(tbl+max_entries, 0);
	}
};


#endif	// #ifndef INCLUDED_ADTS_DYN_HASH_TBL
