#ifndef ADTS_H__
#define ADTS_H__

#include "lib.h"

#include <cassert>

#include <list>
#include <map>



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
template<typename Key, typename T, typename Traits=DHT_Traits<Key,T> > class DynHashTbl
{
	T* tbl;
	u16 num_entries;
	u16 max_entries;	// when initialized, = 2**n for faster modulo
	Traits tr;

	T& get_slot(Key key) const
	{
		size_t hash = tr.hash(key);
		debug_assert(max_entries != 0);	// otherwise, mask will be incorrect
		const uint mask = max_entries-1;
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
		num_entries = 0;
		max_entries = tr.initial_entries/2;	// will be doubled in expand_tbl
		debug_assert(is_pow2(max_entries));
		expand_tbl();
	}

	~DynHashTbl()
	{
		clear();
	}

	void clear()
	{
		free(tbl);
		tbl = 0;
		num_entries = 0;
		// rationale: must not set to 0 because expand_tbl only doubles the size.
		// don't keep the previous size because it may have become huge and
		// there is no provision for shrinking.
		max_entries = tr.initial_entries/2;	// will be doubled in expand_tbl
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



// Cache for items of variable size and value/"cost".
// currently uses Landlord algorithm.

template<typename Key, typename T> class Cache
{
public:
	void add(Key key, T item, size_t size, uint cost)
	{
		typedef std::pair<CacheMapIt, bool> PairIB;
		CacheMap::value_type val = std::make_pair(key, CacheEntry(item, size, cost));
		PairIB ret = map.insert(val);
		debug_assert(ret.second);	// must not already be in map
	}

	T retrieve(Key key, size_t* psize = 0)
	{
		CacheMapIt it = map.find(key);
		if(it == map.end())
			return 0;
		CacheEntry& entry = it->second;
		if(psize)
			*psize = entry.size;
// increase credit
		return entry.item;
	}


	T remove_least_valuable(size_t* psize = 0)
	{
		CacheMapIt it;

again:	// until we find someone to evict

		// foreach entry: decrease credit and evict if <= 0
		for( it = map.begin(); it != map.end(); ++it)
		{
			CacheEntry& entry = it->second;
			// found someone we can evict
			if(entry.credit <= 0.0f)
			{
				T item = entry.item;
				if(psize)
					*psize = entry.size;
				map.erase(it);
				return item;
			}
		}

		// none were evicted
// charge rent
		goto again;
	}

private:
	class CacheEntry
	{
		friend class Cache;

		CacheEntry(T item_, size_t size_, uint cost_)
		{
			item = item_;

			size = size_;
			cost = cost_;
			credit = cost;
		}

		T item;
		size_t size;
		uint cost;
		float credit;
	};

	typedef std::map<Key, CacheEntry> CacheMap;
	typedef typename CacheMap::iterator CacheMapIt;
	CacheMap map;
};



//
// FIFO bit queue
//

struct BitBuf
{
	ulong buf;
	ulong cur;	// bit to be appended (toggled by add())
	ulong len;	// |buf| [bits]

	void reset()
	{
		buf = 0;
		cur = 0;
		len = 0;
	}

	// toggle current bit if desired, and add to buffer (new bit is LSB)
	void add(ulong toggle)
	{
		cur ^= toggle;
		buf <<= 1;
		buf |= cur;
		len++;
	}

	// extract LS n bits
	uint extract(ulong n)
	{
		ulong i = buf & ((1ul << n) - 1);
		buf >>= n;

		return i;
	}
};


//
// ring buffer - static array, accessible modulo n
//

template<class T, size_t n> class RingBuf
{
	size_t size_;	// # of entries in buffer
	size_t head;	// index of first item
	size_t tail;	// index of last  item 
	T data[n];

public:
	RingBuf() { clear(); }
	void clear() { size_ = 0; head = 1; tail = 0; }

	size_t size() { return size_; }
	bool empty() { return size_ == 0; }

	const T& operator[](int ofs) const
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
		if(size_ > 0)
			size_--;
		else
			debug_warn("underflow");

		head = (head + 1) % n;
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

	protected:
		const T* data;
		size_t pos;
			// not mod-N so that begin != end when buffer is full.
	};

	iterator begin()
	{
		return iterator(data, (size_ < n)? 1 : head);
	}
	const_iterator begin() const
	{
		return const_iterator(data, (size_ < n)? 1 : head);
	}
	iterator end()
	{
		return iterator(data, (size_ < n)? size_+1 : head+n);
	}
	const_iterator end() const
	{
		return const_iterator(data, (size_ < n)? size_+1 : head+n);
	}
};



//
// cache
//


// owns a pool of resources (Entry-s), associated with a 64 bit id.
// typical use: add all available resources to the cache via grow();
// assign() ids to the resources, and update the resource data if necessary;
// retrieve() the resource, given id.
template<class Entry> class LRUCache
{
public:
	// 'give' Entry to the cache.
	int grow(Entry& e)
	{
		// add to front of LRU list, but not index
		// (since we don't have an id yet)
		lru_list.push_front(Line(e));
		return 0;
	}


	// find the least-recently used line; associate id with it,
	// and return its Entry. fails (returns 0) if id is already
	// associated, or all lines are locked.
	Entry* assign(u64 id)
	{
		if(find_line(id))
		{
			debug_warn("assign: id already in cache!");
			return 0;
		}

		// scan in least->most used order for first non-locked entry
		List_iterator l = lru_list.end();
		while(l != lru_list.begin())
		{
			--l;
			if(l->refs == 0)
				goto have_line;
		}

		// all are locked and cannot be displaced.
		// caller should grow() enough lines so that this never happens.
		debug_warn("assign: all lines locked - grow() more lines");
		return 0;

	have_line:

		// update mapping (index)
		idx.erase(id);
		idx[id] = l;

		l->id = id;
		return &l->ent;
	}


	// find line identified by id; return its entry or 0 if not in cache.
	Entry* retrieve(u64 id)
	{
		// invalid: id 0 denotes not-yet-associated lines
		if(id == 0)
		{
			debug_warn("retrieve: id 0 not allowed");
			return 0;
		}
		Line* l = find_line(id);
		return l? &l->ent : 0;
	}


	// add/release a reference to a line, to protect it against
	// displacement via associate(). we verify refs >= 0.
	int lock(u64 id, bool locked)
	{
		Line* l = find_line(id);
		if(!l)
			return -1;

		if(locked)
			l->refs++;
		else
		{
			debug_assert(l->refs > 0);
			l->refs--;
		}
		return 0;
	}


private:
	// implementation:
	// cache lines are stored in a list, most recently used in front.
	// a map finds the list entry containing a given id in log-time.

	struct Line
	{
		u64 id;
		Entry ent;
		int refs;	// protect from displacement if > 0

		Line(Entry& _ent)
		{
			id   = 0;
			ent  = _ent;
			refs = 0;
		}
	};

	typedef std::list<Line> List;
	typedef typename List::iterator List_iterator;
	List lru_list;

	typedef std::map<u64, List_iterator> Map;
	Map idx;


	// return the line identified by id, or 0 if not in cache.
	// mark it as the most recently used line.
	Line* find_line(u64 id)
	{
		typename Map::const_iterator i = idx.find(id);
		// not found
		if(i == idx.end())
			return 0;

		// index points us to list entry
		List_iterator l = i->second;

		// mark l as the most recently used line.
		lru_list.splice(lru_list.begin(), lru_list, l);
		idx[l->id] = l;

		return &*l;
	}
};


//
// expansible hash table (linear probing)
//






// from VFS, not currently needed

#if 0
template<class T> class StringMap
{
public:

	T* add(const char* fn, T& t)
	{
		const FnHash fn_hash = fnv_hash(fn);

		t.name = fn;

		std::pair<FnHash, T> item = std::make_pair(fn_hash, t);
		std::pair<MapIt, bool> res;
		res = map.insert(item);

		if(!res.second)
		{
			debug_warn("add: already in container");
			return 0;
		}

		// return address of user data (T) inserted into container.
		return &((res.first)->second);
	}

	T* find(const char* fn)
	{
		const FnHash fn_hash = fnv_hash(fn);
		MapIt it = map.find(fn_hash);
			// O(log(size))
		if(it == map.end())
			return 0;
		return &it->second;
	}

	size_t size() const
	{
		return map.size();
	}

	void clear()
	{
		map.clear();
	}


private:
	typedef std::map<FnHash, T> Map;
	typedef typename Map::iterator MapIt;
	Map map;


public:

	class iterator
	{
	public:
		iterator()
			{}
		iterator(typename StringMap<T>::MapIt _it)
			{ it = _it; }
		T& operator*() const
			{ return it->second; }
		T* operator->() const
			{ return &**this; }
		iterator& operator++()	// pre
			{ ++it; return (*this); }
		bool operator==(const iterator& rhs) const
			{ return it == rhs.it; }
		bool operator!=(const iterator& rhs) const
		{ return !(*this == rhs); }
	protected:
		typename StringMap<T>::MapIt it;
	};

	iterator begin()
		{ return iterator(map.begin()); }

	iterator end()
		{ return iterator(map.end()); }

};



template<class Key, class Data> class PriMap
{
public:

	int add(Key key, uint pri, Data& data)
	{
		Item item = std::make_pair(pri, data);
		MapEntry ent = std::make_pair(key, item);
		std::pair<MapIt, bool> ret;
		ret = map.insert(ent);
		// already in map
		if(!ret.second)
		{
			MapIt it = ret.first;
			Item item = it->second;
			const uint old_pri = item.first;
			Data& old_data     = item.second;

			// new data is of higher priority; replace older data
			if(old_pri <= pri)
			{
				old_data = data;
				return 0;
			}
			// new data is of lower priority; don't add
			else
				return 1;
		}

		return 0;
	}

	Data* find(Key key)
	{
		MapIt it = map.find(key);
		if(it == map.end())
			return 0;

		return &it->second.second;
	}

	void clear()
	{
		map.clear();
	}

private:
	typedef std::pair<uint, Data> Item;
	typedef std::pair<Key, Item> MapEntry;
	typedef std::map<Key, Item> Map;
	typedef typename Map::iterator MapIt;
	Map map;
};
#endif // #if 0



#endif	// #ifndef ADTS_H__
