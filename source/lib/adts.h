#ifndef ADTS_H__
#define ADTS_H__

#include "lib.h"

#include <cassert>

#include <list>
#include <map>


struct BIT_BUF
{
	ulong buf;
	ulong cur;	/* bit to be appended (toggled by add()) */
	ulong len;	/* |buf| [bits] */

	void reset()
	{
		buf = 0;
		cur = 0;
		len = 0;
	}

	/* toggle current bit if desired, and add to buffer (new bit is LSB) */
	void add(ulong toggle)
	{
		cur ^= toggle;
		buf <<= 1;
		buf |= cur;
		len++;
	}

	/* extract LS n bits */
	uint extract(ulong n)
	{
		ulong i = buf & ((1ul << n) - 1);
		buf >>= n;

		return i;
	}
};


template<class T, int n> struct RingBuf
{
	size_t size_;	// # of entries in buffer
	size_t pos;		// index of oldest data
	T data[n];

	RingBuf() { clear(); }
	void clear() { size_ = 0; pos = 0; }
	size_t size() { return size_; }

	const T& operator[](int idx) const
	{
		assert(idx < n);
		return data[idx];
	}

	void push_back(const T& item)
	{
		if(size_ < n)
			size_++;

		data[pos] = item;
		pos = (pos + 1) % n;
	}

	class const_iterator
	{
	public:
		const_iterator() : data(0), pos(0)
			{}
		const_iterator(const T* _data, size_t _pos) : data(_data), pos(_pos)
			{}
		const T& operator[](int idx) const
			{ return data[(pos+idx) % n]; }
		const T& operator*() const
			{ return data[pos]; }
		const T* operator->() const
			{ return &**this; }
		const_iterator& operator++()	// pre
			{ pos = (pos+1) % n; return (*this); }
		const_iterator operator++(int)	// post
			{ const_iterator tmp =  *this; ++*this; return tmp; }
		bool operator==(const const_iterator& rhs) const
			{ return pos == rhs.pos && data == rhs.data; }
		bool operator!=(const const_iterator& rhs) const
			{ return !(*this == rhs); }
	protected:
		const T* data;
		size_t pos;
	};

	const_iterator begin() const
	{ return const_iterator(data, (size_ < n)? 0 : pos); }

	const_iterator end() const
	{ return const_iterator(data, (size_ < n)? size_ : pos); }
};



//
// cache
//


// owns a pool of resources (Entry-s), associated with a 64 bit id.
// typical use: add all available resources to the cache via grow();
// assign() ids to the resources, and update the resource data if necessary;
// retrieve() the resource, given id.
template<class Entry> class Cache
{
public:
	// 'give' Entry to the cache.
	int grow(Entry& e)
	{
		// add to front of LRU list, but not index
		// (since we don't have an id yet)
		lru_list.push_front(Line(0, e));
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
			assert(l->refs > 0);
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

		Line(u64 _tag, Entry& _ent)
		{
			id  = 0;
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
		Map::const_iterator i = idx.find(id);
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


// from VFS, not currently needed

/*
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
*/



#endif	// #ifndef ADTS_H__
