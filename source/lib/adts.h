/**
 * =========================================================================
 * File        : adts.h
 * Project     : 0 A.D.
 * Description : useful Abstract Data Types not provided by STL.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADTS_H__
#define ADTS_H__

#include "lib.h"

#include <cfloat>
#include <cassert>

#include <list>
#include <map>
#include <queue>


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
		// note: users might call clear() right before the dtor runs,
		// so safely handling calling this twice.
		SAFE_FREE(tbl);
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


//-----------------------------------------------------------------------------

/*
Cache for items of variable size and value/"cost".
underlying displacement algorithm is pluggable; default is "Landlord".

template reference:
Entry provides size, cost, credit and credit_density().
  rationale:
  - made a template instead of exposing Cache::Entry because
    that would drag a lot of stuff out of Cache.
  - calculates its own density since that entails a Divider functor,
    which requires storage inside Entry.
Entries is a collection with iterator and begin()/end() and
  "static Entry& entry_from_it(iterator)".
  rationale:
  - STL map has pair<key, item> as its value_type, so this
    function would return it->second. however, we want to support
    other container types (where we'd just return *it).
Manager is a template parameterized on typename Key and class Entry.
  its interface is as follows:

	// is the cache empty?
	bool empty() const;

	// add (key, entry) to cache.
	void add(Key key, const Entry& entry);

	// if the entry identified by <key> is not in cache, return false;
	// otherwise return true and pass back a pointer to it.
	bool find(Key key, const Entry** pentry) const;

	// remove an entry from cache, which is assumed to exist!
	// this makes sense because callers will typically first use find() to
	// return info about the entry; this also checks if present.
	void remove(Key key);

	// mark <entry> as just accessed for purpose of cache management.
	// it will tend to be kept in cache longer.
	void on_access(Entry& entry);

	// caller's intent is to remove the least valuable entry.
	// in implementing this, you have the latitude to "shake loose"
	// several entries (e.g. because their 'value' is equal).
	// they must all be push_back-ed into the list; Cache will dole
	// them out one at a time in FIFO order to callers.
	//
	// rationale:
	// - it is necessary for callers to receive a copy of the
	//   Entry being evicted - e.g. file_cache owns its items and
	//   they must be passed back to allocator when evicted.
	// - e.g. Landlord can potentially see several entries become
	//   evictable in one call to remove_least_valuable. there are
	//   several ways to deal with this:
	//   1) generator interface: we return one of { empty, nevermind,
	//      removed, remove-and-call-again }. this greatly complicates
	//      the call site.
	//   2) return immediately after finding an item to evict.
	//      this changes cache behavior - entries stored at the
	//      beginning would be charged more often (unfair).
	//      resuming charging at the next entry doesn't work - this
	//      would have to be flushed when adding, at which time there
	//      is no provision for returning any items that may be evicted.
	//   3) return list of all entries "shaken loose". this incurs
	//      frequent mem allocs, which can be alleviated via suballocator.
	//      note: an intrusive linked-list doesn't make sense because
	//      entries to be returned need to be copied anyway (they are
	//      removed from the manager's storage).
	void remove_least_valuable(std::list<Entry>& entry_list)
*/


//
// functors to calculate minimum credit density (MCD)
//

// MCD is required for the Landlord algorithm's evict logic.
// [Young02] calls it '\delta'.

// scan over all entries and return MCD.
template<class Entries> float ll_calc_min_credit_density(const Entries& entries)
{
	float min_credit_density = FLT_MAX;
	for(typename Entries::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
		const float credit_density = Entries::entry_from_it(it).credit_density();
		min_credit_density = fminf(min_credit_density, credit_density);
	}
	return min_credit_density;
}

// note: no warning is given that the MCD entry is being removed!
// (reduces overhead in remove_least_valuable)
// these functors must account for that themselves (e.g. by resetting
// their state directly after returning MCD).

// determine MCD by scanning over all entries.
// tradeoff: O(N) time complexity, but all notify* calls are no-ops.
template<class Entry, class Entries>
class McdCalc_Naive
{
public:
	void notify_added(const Entry&) const {}
	void notify_decreased(const Entry&) const {}
	void notify_impending_increase_or_remove(const Entry&) const {}
	void notify_increased_or_removed(const Entry&) const {}
	float operator()(const Entries& entries) const
	{
		const float mcd = ll_calc_min_credit_density(entries);
		return mcd;
	}
};

// cache previous MCD and update it incrementally (when possible).
// tradeoff: amortized O(1) time complexity, but notify* calls must
// perform work whenever something in the cache changes.
template<class Entry, class Entries>
class McdCalc_Cached
{
public:
	McdCalc_Cached() : min_credit_density(FLT_MAX), min_valid(false) {}

	void notify_added(const Entry& entry)
	{
		// when adding a new item, the minimum credit density can only
		// decrease or remain the same; acting as if entry's credit had
		// been decreased covers both cases.
		notify_decreased(entry);
	}

	void notify_decreased(const Entry& entry)
	{
		min_credit_density = MIN(min_credit_density, entry.credit_density());
	}

	void notify_impending_increase_or_remove(const Entry& entry)
	{
		// remember if this entry had the smallest density
		is_min_entry = feq(min_credit_density, entry.credit_density());
	}

	void notify_increased_or_removed(const Entry& UNUSED(entry))
	{
		// .. it did and was increased or removed. we must invalidate
		// MCD and recalculate it next time.
		if(is_min_entry)
		{
			min_valid = false;
			min_credit_density = -1.0f;
		}
	}

	float operator()(const Entries& entries)
	{
		if(min_valid)
		{
			// the entry that has MCD will be removed anyway by caller;
			// we need to invalidate here because they don't call
			// notify_increased_or_removed.
			min_valid = false;
			return min_credit_density;
		}

		// this is somewhat counterintuitive. since we're calculating
		// MCD directly, why not mark our cached version of it valid
		// afterwards? reason is that our caller will remove the entry with
		// MCD, so it'll be invalidated anyway.
		// instead, our intent is to calculate MCD for the *next time*.
		const float ret = ll_calc_min_credit_density(entries);
		min_valid = true;
		min_credit_density = FLT_MAX;
		return ret;
	}

private:
	float min_credit_density;
	bool min_valid;

	// temporary flag set by notify_impending_increase_or_remove
	bool is_min_entry;
};


//
// Landlord cache management policy: see [Young02].
//

// in short, each entry has credit initially set to cost. when wanting to
// remove an item, all are charged according to MCD and their size;
// entries are evicted if their credit is exhausted. accessing an entry
// restores "some" of its credit.
template<typename Key, typename Entry, template<class Entry, class Entries> class McdCalc = McdCalc_Cached>
class Landlord
{
public:
	bool empty() const
	{
		return map.empty();
	}

	void add(Key key, const Entry& entry)
	{
		// adapter for add_ (which returns an iterator)
		(void)add_(key, entry);
	}

	bool find(Key key, const Entry** pentry) const
	{
		MapCIt it = map.find(key);
		if(it == map.end())
			return false;
		*pentry = &it->second;
		return true;
	}

	void remove(Key key)
	{
		MapIt it = map.find(key);
		debug_assert(it != map.end());
		remove_(it);
	}

	void on_access(Entry& entry)
	{
		mcd_calc.notify_impending_increase_or_remove(entry);

		// Landlord algorithm calls for credit to be reset to anything
		// between its current value and the cost.
		const float gain = 0.75f;	// restore most credit
		entry.credit = gain*entry.cost + (1.0f-gain)*entry.credit;

		mcd_calc.notify_increased_or_removed(entry);
	}

	void remove_least_valuable(std::list<Entry>& entry_list)
	{
		// we are required to evict at least one entry. one iteration
		// ought to suffice, due to definition of min_credit_density and
		// tolerance; however, we provide for repeating if necessary.
again:

		// messing with this (e.g. raising if tiny) would result in
		// different evictions than Landlord_Lazy, which is unacceptable.
		// nor is doing so necessary: if mcd is tiny, so is credit.
		const float min_credit_density = mcd_calc(map);
		debug_assert(min_credit_density > 0.0f);

		for(MapIt it = map.begin(); it != map.end();)	// no ++it
		{
			Entry& entry = it->second;

			charge(entry, min_credit_density);
			if(should_evict(entry))
			{
				entry_list.push_back(entry);

				// annoying: we have to increment <it> before erasing
				MapIt it_to_remove = it++;
				map.erase(it_to_remove);
			}
			else
			{
				mcd_calc.notify_decreased(entry);
				++it;
			}
		}

		if(entry_list.empty())
			goto again;
	}

protected:
	// note: use hash_map instead of map for better locality
	// (relevant when iterating over all items in remove_least_valuable)
	class Map : public STL_HASH_MAP<Key, Entry>
	{
	public:
		static Entry& entry_from_it(typename Map::iterator it) { return it->second; }
		static const Entry& entry_from_it(typename Map::const_iterator it) { return it->second; }
	};
	typedef typename Map::iterator MapIt;
	typedef typename Map::const_iterator MapCIt;
	Map map;

	// add entry and return iterator pointing to it.
	MapIt add_(Key key, const Entry& entry)
	{
		typedef std::pair<MapIt, bool> PairIB;
		typename Map::value_type val = std::make_pair(key, entry);
		PairIB ret = map.insert(val);
		debug_assert(ret.second);	// must not already be in map

		mcd_calc.notify_added(entry);

		return ret.first;
	}

	// remove entry (given by iterator) directly.
	void remove_(MapIt it)
	{
		const Entry& entry = it->second;
		mcd_calc.notify_impending_increase_or_remove(entry);
		mcd_calc.notify_increased_or_removed(entry);
		map.erase(it);
	}

	void charge(Entry& entry, float delta)
	{
		entry.credit -= delta * entry.size;

		// don't worry about entry.size being 0 - if so, cost
		// should also be 0, so credit will already be 0 anyway.
	}

	// for each entry, 'charge' it (i.e. reduce credit by) delta * its size.
	// delta is typically MCD (see above); however, several such updates
	// may be lumped together to save time. Landlord_Lazy does this.
	void charge_all(float delta)
	{
		for(MapIt it = map.begin(); it != map.end(); ++it)
		{
			Entry& entry = it->second;
			entry.credit -= delta * entry.size;
			if(!should_evict(entry))
				mcd_calc.notify_decreased(entry);
		}
	}

	// is entry's credit exhausted? if so, it should be evicted.
	bool should_evict(const Entry& entry)
	{
		// we need a bit of leeway because density calculations may not
		// be exact. choose value carefully: must not be high enough to
		// trigger false positives.
		return entry.credit < 0.0001f;
	}

private:
	McdCalc<Entry, Map> mcd_calc;
};

// Cache manger policies. (these are partial specializations of Landlord,
// adapting it to the template params required by Cache)
template<class Key, class Entry> class Landlord_Naive : public Landlord<Key, Entry, McdCalc_Naive> {};
template<class Key, class Entry> class Landlord_Cached: public Landlord<Key, Entry, McdCalc_Cached> {};

// variant of Landlord that adds a priority queue to directly determine
// which entry to evict. this allows lumping several charge operations
// together and thus reduces iteration over all entries.
// tradeoff: O(logN) removal (instead of N), but additional O(N) storage.
template<typename Key, class Entry>
class Landlord_Lazy : public Landlord_Naive<Key, Entry>
{
	typedef typename Landlord_Naive<Key, Entry>::Map Map;
	typedef typename Landlord_Naive<Key, Entry>::MapIt MapIt;
	typedef typename Landlord_Naive<Key, Entry>::MapCIt MapCIt;

public:
	Landlord_Lazy() { pending_delta = 0.0f; }

	void add(Key key, const Entry& entry)
	{
		// we must apply pending_delta now - otherwise, the existing delta
		// would later be applied to this newly added item (incorrect).
		commit_pending_delta();

		MapIt it = Parent::add_(key, entry);
		pri_q.push(it);
	}

	void remove(Key key)
	{
		Parent::remove(key);

		// reconstruct pri_q from current map. this is slow (N*logN) and
		// could definitely be done better, but we don't bother since
		// remove is a very rare operation (e.g. invalidating entries).
		while(!pri_q.empty())
			pri_q.pop();
		for(MapCIt it = this->map.begin(); it != this->map.end(); ++it)
			pri_q.push(it);
	}

	void on_access(Entry& entry)
	{
		Parent::on_access(entry);

		// entry's credit was changed. we now need to reshuffle the
		// pri queue to reflect this.
		pri_q.ensure_heap_order();
	}

	void remove_least_valuable(std::list<Entry>& entry_list)
	{
		MapIt least_valuable_it = pri_q.top(); pri_q.pop();
		Entry& entry = Map::entry_from_it(least_valuable_it);

		entry_list.push_back(entry);

		// add to pending_delta the MCD that would have resulted
		// if removing least_valuable_it normally.
		// first, calculate actual credit (i.e. apply pending_delta to
		// this entry); then add the resulting density to pending_delta.
		entry.credit -= pending_delta*entry.size;
		const float credit_density = entry.credit_density();
		debug_assert(credit_density > 0.0f);
		pending_delta += credit_density;

		Parent::remove_(least_valuable_it);
	}

private:
	typedef Landlord_Naive<Key, Entry> Parent;

	// sort iterators by credit_density of the Entry they reference.
	struct CD_greater
	{
		bool operator()(MapIt it1, MapIt it2) const
		{
			return Map::entry_from_it(it1).credit_density() >
			       Map::entry_from_it(it2).credit_density();
		}
	};
	// wrapper on top of priority_queue that allows 'heap re-sift'
	// (see on_access).
	// notes:
	// - greater comparator makes pri_q.top() the one with
	//   LEAST credit_density, which is what we want.
	// - deriving from an STL container is a bit dirty, but we need this
	//   to get at the underlying data (priority_queue interface is not
	//   very capable).
	class PriQ: public std::priority_queue<MapIt, std::vector<MapIt>, CD_greater>
	{
	public:
		void ensure_heap_order()
		{
			// TODO: this is actually N*logN - ouch! that explains high
			// CPU cost in profile. this is called after only 1 item has
			// changed, so a logN "sift" operation ought to suffice.
			// that's not supported by the STL heap functions, so we'd
			// need a better implementation. pending..
			std::make_heap(this->c.begin(), this->c.end(), this->comp);
		}
	};
	PriQ pri_q;

	// delta values that have accumulated over several
	// remove_least_valuable() calls. applied during add().
	float pending_delta;

	void commit_pending_delta()
	{
		if(pending_delta > 0.0f)
		{
			this->charge_all(pending_delta);
			pending_delta = 0.0f;

			// we've changed entry credit, so the heap order *may* have been
			// violated; reorder the pri queue. (I don't think so,
			// due to definition of delta, but we'll play it safe)
			pri_q.ensure_heap_order();
		}
	}
};


//
// functor that implements division of first arg by second
//

// this is used to calculate credit_density(); performance matters
// because this is called for each entry during each remove operation.

// floating-point division (fairly slow)
class Divider_Naive
{
public:
	Divider_Naive() {}	// needed for default CacheEntry ctor
	Divider_Naive(float UNUSED(x)) {}
	float operator()(float val, float divisor) const
	{
		return val / divisor;
	}
};

// caches reciprocal of divisor and multiplies by that.
// tradeoff: only 4 clocks (instead of 20), but 4 bytes extra per entry.
class Divider_Recip
{
	float recip;
public:
	Divider_Recip() {}	// needed for default CacheEntry ctor
	Divider_Recip(float x) { recip = 1.0f / x; }
	float operator()(float val, float UNUSED(divisor)) const
	{
		return val * recip;
	}
};

// TODO: use SSE/3DNow RCP instruction? not yet, because not all systems
// support it and overhead of detecting this support eats into any gains.

// initial implementation for testing purposes; quite inefficient.
template<typename Key, typename Entry>
class LRU
{
public:
	bool empty() const
	{
		return lru.empty();
	}

	void add(Key key, const Entry& entry)
	{
		lru.push_back(KeyAndEntry(key, entry));
	}

	bool find(Key key, const Entry** pentry) const
	{
		CIt it = std::find_if(lru.begin(), lru.end(), KeyEq(key));
		if(it == lru.end())
			return false;
		*pentry = &it->entry;
		return true;
	}

	void remove(Key key)
	{
		std::remove_if(lru.begin(), lru.end(), KeyEq(key));
	}

	void on_access(Entry& entry)
	{
		for(It it = lru.begin(); it != lru.end(); ++it)
		{
			if(&entry == &it->entry)
			{
				add(it->key, it->entry);
				lru.erase(it);
				return;
			}
		}
		debug_warn("entry not found in list");
	}

	void remove_least_valuable(std::list<Entry>& entry_list)
	{
		entry_list.push_back(lru.front().entry);
		lru.pop_front();
	}

private:
	struct KeyAndEntry
	{
		Key key;
		Entry entry;
		KeyAndEntry(Key key_, const Entry& entry_)
			: key(key_), entry(entry_) {}
	};
	class KeyEq
	{
		Key key;
	public:
		KeyEq(Key key_) : key(key_) {}
		bool operator()(const KeyAndEntry& ke) const
		{
			return ke.key == key;
		}
	};

	typedef std::list<KeyAndEntry> List;
	typedef typename List::iterator It;
	typedef typename List::const_iterator CIt;
	List lru;
};


//
// Cache
//

template
<
typename Key, typename Item,
// see documentation above for Manager's interface.
template<typename Key, class Entry> class Manager = Landlord_Cached,
class Divider = Divider_Naive
>
class Cache
{
public:
	Cache() : mgr() {}

	void add(Key key, Item item, size_t size, uint cost)
	{
		return mgr.add(key, Entry(item, size, cost));
	}

	// remove the entry identified by <key>. expected usage is to check
	// if present and determine size via retrieve(), so no need for
	// error checking.
	// useful for invalidating single cache entries.
	void remove(Key key)
	{
		mgr.remove(key);
	}

	// if there is no entry for <key> in the cache, return false.
	// otherwise, return true and pass back item and (optionally) size.
	//
	// if refill_credit (default), the cache manager 'rewards' this entry,
	// tending to keep it in cache longer. this parameter is not used in
	// normal operation - it's only for special cases where we need to
	// make an end run around the cache accounting (e.g. for cache simulator).
	bool retrieve(Key key, Item& item, size_t* psize = 0, bool refill_credit = true)
	{
		const Entry* entry;
		if(!mgr.find(key, &entry))
			return false;

		item = entry->item;
		if(psize)
			*psize = entry->size;

		if(refill_credit)
			mgr.on_access((Entry&)*entry);

		return true;
	}

	// toss out the least valuable entry. return false if cache is empty,
	// otherwise true and (optionally) pass back its item and size.
	bool remove_least_valuable(Item* pItem = 0, size_t* pSize = 0)
	{
		// as an artefact of the cache eviction policy, several entries
		// may be "shaken loose" by one call to remove_least_valuable.
		// we cache them in a list to disburden callers (they always get
		// exactly one).
		if(entries_awaiting_eviction.empty())
		{
			if(empty())
				return false;

			mgr.remove_least_valuable(entries_awaiting_eviction);
			debug_assert(!entries_awaiting_eviction.empty());
		}

		const Entry& entry = entries_awaiting_eviction.front();
		if(pItem)
			*pItem = entry.item;
		if(pSize)
			*pSize = entry.size;
		entries_awaiting_eviction.pop_front();

		return true;
	}

	bool empty() const
	{
		return mgr.empty();
	}

private:
	// this is applicable to all cache management policies and stores all
	// required information. a Divider functor is used to implement
	// division for credit_density.
	template<class InnerDivider> struct CacheEntry
	{
		Item item;
		size_t size;
		uint cost;
		float credit;

		InnerDivider divider;

		// needed for mgr.remove_least_valuable's entry_copy
		CacheEntry() {}

		CacheEntry(Item item_, size_t size_, uint cost_)
			: item(item_), divider((float)size_)
		{
			size = size_;
			cost = cost_;
			credit = cost;

			// else divider will fail
			debug_assert(size != 0);
		}

		float credit_density() const
		{
			return divider(credit, (float)size);
		}
	};
	typedef CacheEntry<Divider> Entry;

	// see note in remove_least_valuable().
	std::list<Entry> entries_awaiting_eviction;

	Manager<Key, Entry> mgr;
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
			debug_warn("underflow");
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

#endif	// #ifndef ADTS_H__
