/* Copyright (C) 2009 Wildfire Games.
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

/**
 * =========================================================================
 * File        : adts.h
 * Project     : 0 A.D.
 * Description : useful Abstract Data Types not provided by STL.
 * =========================================================================
 */

#ifndef INCLUDED_ADTS
#define INCLUDED_ADTS

#include "lib/fnv_hash.h"
#include "lib/bits.h"


//-----------------------------------------------------------------------------
// dynamic (grow-able) hash table
//-----------------------------------------------------------------------------

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
		debug_assert(max_entries != 0);	// otherwise, mask will be incorrect
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

		max_entries += max_entries;
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
		debug_assert(slot == 0);	// not already present
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


//-----------------------------------------------------------------------------
// FIFO bit queue
//-----------------------------------------------------------------------------

struct BitBuf
{
	uintptr_t buf;
	uintptr_t cur;	// bit to be appended (toggled by add())
	size_t len;	// |buf| [bits]

	void reset()
	{
		buf = 0;
		cur = 0;
		len = 0;
	}

	// toggle current bit if desired, and add to buffer (new bit is LSB)
	void add(uintptr_t toggle)
	{
		cur ^= toggle;
		buf <<= 1;
		buf |= cur;
		len++;
	}

	// extract LS n bits
	size_t extract(uintptr_t n)
	{
		const uintptr_t bits = buf & bit_mask<uintptr_t>(n);
		buf >>= n;

		return bits;
	}
};


//-----------------------------------------------------------------------------
// ring buffer - static array, accessible modulo n
//-----------------------------------------------------------------------------

template<class T, size_t n> class RingBuf
{
	size_t size_;	// # of entries in buffer
	size_t head;	// index of oldest item
	size_t tail;	// index of newest item
	T data[n];

public:
	RingBuf() : data() { clear(); }
	void clear() { size_ = 0; head = 0; tail = n-1; }

	size_t size() { return size_; }
	bool empty() { return size_ == 0; }

	const T& operator[](int ofs) const
	{
		debug_assert(!empty());
		size_t idx = (size_t)(head + ofs);
		return data[idx % n];
	}
	T& operator[](int ofs)
	{
		debug_assert(!empty());
		size_t idx = (size_t)(head + ofs);
		return data[idx % n];
	}

	T& front()
	{
		debug_assert(!empty());
		return data[head];
	}
	const T& front() const
	{
		debug_assert(!empty());
		return data[head];
	}
	T& back()
	{
		debug_assert(!empty());
		return data[tail];
	}
	const T& back() const
	{
		debug_assert(!empty());
		return data[tail];
	}

	void push_back(const T& item)
	{
		if(size_ < n)
			size_++;
		// do not complain - overwriting old values is legit
		// (e.g. sliding window).
		else
			head = (head + 1) % n;

		tail = (tail + 1) % n;
		data[tail] = item;
	}

	void pop_front()
	{
		if(size_ != 0)
		{
			size_--;
			head = (head + 1) % n;
		}
		else
			debug_assert(0);	// underflow
	}

	class iterator
	{
	public:
		typedef std::random_access_iterator_tag iterator_category;
		typedef T value_type;
		typedef ptrdiff_t difference_type;
		typedef T* pointer;
		typedef T& reference;

		iterator() : data(0), pos(0)
			{}
		iterator(T* data_, size_t pos_) : data(data_), pos(pos_)
			{}
		T& operator[](int idx) const
			{ return data[(pos+idx) % n]; }
		T& operator*() const
			{ return data[pos % n]; }
		T* operator->() const
			{ return &**this; }
		iterator& operator++()	// pre
			{ ++pos; return (*this); }
		iterator operator++(int)	// post
			{ iterator tmp =  *this; ++*this; return tmp; }
		bool operator==(const iterator& rhs) const
			{ return data == rhs.data && pos == rhs.pos; }
		bool operator!=(const iterator& rhs) const
			{ return !(*this == rhs); }
		bool operator<(const iterator& rhs) const
			{ return (pos < rhs.pos); }
		iterator& operator+=(difference_type ofs)
			{ pos += ofs; return *this; }
		iterator& operator-=(difference_type ofs)
			{ return (*this += -ofs); }
		iterator operator+(difference_type ofs) const
			{ iterator tmp = *this; return (tmp += ofs); }
		iterator operator-(difference_type ofs) const
			{ iterator tmp = *this; return (tmp -= ofs); }
		difference_type operator-(const iterator right) const
			{ return (difference_type)(pos - right.pos); }

	protected:
		T* data;
		size_t pos;
		// not mod-N so that begin != end when buffer is full.
	};

	class const_iterator
	{
	public:
		typedef std::random_access_iterator_tag iterator_category;
		typedef T value_type;
		typedef ptrdiff_t difference_type;
		typedef const T* pointer;
		typedef const T& reference;

		const_iterator() : data(0), pos(0)
			{}
		const_iterator(const T* data_, size_t pos_) : data(data_), pos(pos_)
			{}
		const T& operator[](int idx) const
			{ return data[(pos+idx) % n]; }
		const T& operator*() const
			{ return data[pos % n]; }
		const T* operator->() const
			{ return &**this; }
		const_iterator& operator++()	// pre
			{ ++pos; return (*this); }
		const_iterator operator++(int)	// post
			{ const_iterator tmp =  *this; ++*this; return tmp; }
		bool operator==(const const_iterator& rhs) const
			{ return data == rhs.data && pos == rhs.pos; }
		bool operator!=(const const_iterator& rhs) const
			{ return !(*this == rhs); }
		bool operator<(const const_iterator& rhs) const
			{ return (pos < rhs.pos); }
		iterator& operator+=(difference_type ofs)
			{ pos += ofs; return *this; }
		iterator& operator-=(difference_type ofs)
			{ return (*this += -ofs); }
		iterator operator+(difference_type ofs) const
			{ iterator tmp = *this; return (tmp += ofs); }
		iterator operator-(difference_type ofs) const
			{ iterator tmp = *this; return (tmp -= ofs); }
		difference_type operator-(const iterator right) const
			{ return (difference_type)(pos - right.pos); }

	protected:
		const T* data;
		size_t pos;
			// not mod-N so that begin != end when buffer is full.
	};

	iterator begin()
	{
		return iterator(data, (size_ < n)? 0 : head);
	}
	const_iterator begin() const
	{
		return const_iterator(data, (size_ < n)? 0 : head);
	}
	iterator end()
	{
		return iterator(data, (size_ < n)? size_ : head+n);
	}
	const_iterator end() const
	{
		return const_iterator(data, (size_ < n)? size_ : head+n);
	}
};


// Matei's slightly friendlier hashtable for value-type keys
template<typename K, typename T, typename HashCompare >
class MateiHashTbl
{
	static const size_t initial_entries = 16;

	struct Entry {
		bool valid;
		K key;
		T value;
		Entry() : valid(false) {}
		Entry(const K& k, T v) { key=k; value=v; }
		Entry& operator=(const Entry& other) {
			valid = other.valid;
			key = other.key;
			value = other.value;
			return *this;
		}
	};

	Entry* tbl;
	u16 num_entries;
	u16 max_entries;	// when initialized, = 2**n for faster modulo
	HashCompare hashFunc;

	Entry& get_slot(K key) const
	{
		size_t hash = hashFunc(key);
		//debug_assert(max_entries != 0);	// otherwise, mask will be incorrect
		const size_t mask = max_entries-1;
		int stride = 1;	// for quadratic probing
		for(;;)
		{
			Entry& e = tbl[hash & mask];
			// empty slot encountered => not found
			if(!e.valid)
				return e;
			// keys are actually equal => found it
			if(e.key == key)
				return e;
			// keep going (quadratic probing)
			hash += stride;
			stride++;
		}
	}

	void expand_tbl()
	{
		// alloc a new table (but don't assign it to <tbl> unless successful)
		Entry* old_tbl = tbl;
		tbl = new Entry[max_entries*2];
		if(!tbl)
		{
			tbl = old_tbl;
			throw std::bad_alloc();
		}

		max_entries += max_entries;
		// must be set before get_slot

		// newly initialized, nothing to copy - done
		if(!old_tbl)
			return;

		// re-hash from old table into the new one
		for(size_t i = 0; i < max_entries/2u; i++)
		{
			Entry& e = old_tbl[i];
			if(e.valid)
				get_slot(e.key) = e;
		}
		delete[] old_tbl;
	}

	void delete_contents()
	{
		if(tbl) 
		{ 
			delete[] tbl; 
			tbl = 0; 
		}
	}

public:

	MateiHashTbl()
	{
		tbl = 0;
		num_entries = 0;
		max_entries = initial_entries/2;	// will be doubled in expand_tbl
		//debug_assert(is_pow2(max_entries));
		expand_tbl();
	}

	~MateiHashTbl()
	{
		delete_contents();
	}

	void clear()
	{
		delete_contents();
		num_entries = 0;
		// rationale: must not set to 0 because expand_tbl only doubles the size.
		// don't keep the previous size because it may have become huge and
		// there is no provision for shrinking.
		max_entries = initial_entries/2;	// will be doubled in expand_tbl
		expand_tbl();
	}

	bool contains(const K& key) const 
	{
		return get_slot(key).valid;
	}

	T& operator[](const K& key)
	{
		Entry* slot = &get_slot(key);
		if(slot->valid) 
		{
			return slot->value;
		}

		// no element exists for this key - insert it into the table
		// (this is slightly different from STL::hash_map in that we insert a new element
		// on a get for a nonexistent key, but hopefully that's not a problem)

		// if more than 75% full, increase table size and find slot again
		if(num_entries*4 >= max_entries*2)
		{
			expand_tbl();
			slot = &get_slot(key);	// find slot again since we expanded
		}

		slot->valid = true;
		slot->key = key;
		num_entries++;
		return slot->value;
	}

	size_t size() const
	{
		return num_entries;
	}

	// Not an STL iterator, more like a Java one
	// Usage: for(HashTable::Iterator it(table); it.valid(); it.advance()) { do stuff to it.key() and it.value() }
	class Iterator 
	{
	private:
		Entry* pos;
		Entry* end;
		
	public:
		Iterator(const MateiHashTbl& ht)
		{
			pos = ht.tbl;
			end = ht.tbl + ht.max_entries;
			while(pos < end && !pos->valid)
				pos++;
		};

		bool valid() const
		{
			return pos < end;
		}

		void advance()
		{
			do {
				pos++;
			}
			while(pos < end && !pos->valid);
		}

		const K& key() 
		{
			return pos->key;
		}
		
		T& value() 
		{
			return pos->value;
		}
	};
};


#endif	// #ifndef INCLUDED_ADTS
