/* Copyright (C) 2010 Wildfire Games.
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
 * Customizable cache data structure.
 */

#ifndef INCLUDED_CACHE_ADT
#define INCLUDED_CACHE_ADT

#include <cfloat>

#include <list>
#include <map>
#include <queue> // std::priority_queue

#if CONFIG_ENABLE_BOOST
# include <boost/unordered_map.hpp>
# define MAP boost::unordered_map
#else
# define MAP stdext::hash_map
#endif

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
	void add(const Key& key, const Entry& entry);

	// if the entry identified by <key> is not in cache, return false;
	// otherwise return true and pass back a pointer to it.
	bool find(const Key& key, const Entry** pentry) const;

	// remove an entry from cache, which is assumed to exist!
	// this makes sense because callers will typically first use find() to
	// return info about the entry; this also checks if present.
	void remove(const Key& key);

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
		min_credit_density = std::min(min_credit_density, credit_density);
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
		min_credit_density = std::min(min_credit_density, entry.credit_density());
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
template<typename Key, typename Entry, template<class Entry_, class Entries> class McdCalc = McdCalc_Cached>
class Landlord
{
public:
	bool empty() const
	{
		return map.empty();
	}

	void add(const Key& key, const Entry& entry)
	{
		// adapter for add_ (which returns an iterator)
		(void)add_(key, entry);
	}

	bool find(const Key& key, const Entry** pentry) const
	{
		MapCIt it = map.find(key);
		if(it == map.end())
			return false;
		*pentry = &it->second;
		return true;
	}

	void remove(const Key& key)
	{
		MapIt it = map.find(key);
		// note: don't complain if not in the cache: this happens after
		// writing a file and invalidating its cache entry, which may
		// or may not exist.
		if(it != map.end())
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
		ENSURE(min_credit_density > 0.0f);

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
	class Map : public MAP<Key, Entry>
	{
	public:
		static Entry& entry_from_it(typename Map::iterator it) { return it->second; }
		static const Entry& entry_from_it(typename Map::const_iterator it) { return it->second; }
	};
	typedef typename Map::iterator MapIt;
	typedef typename Map::const_iterator MapCIt;
	Map map;

	// add entry and return iterator pointing to it.
	MapIt add_(const Key& key, const Entry& entry)
	{
		typedef std::pair<MapIt, bool> PairIB;
		typename Map::value_type val = std::make_pair(key, entry);
		PairIB ret = map.insert(val);
		ENSURE(ret.second);	// must not already be in map

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

	void add(const Key& key, const Entry& entry)
	{
		// we must apply pending_delta now - otherwise, the existing delta
		// would later be applied to this newly added item (incorrect).
		commit_pending_delta();

		MapIt it = Parent::add_(key, entry);
		pri_q.push(it);
	}

	void remove(const Key& key)
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
		ENSURE(credit_density > 0.0f);
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


// initial implementation for testing purposes; quite inefficient.
template<typename Key, typename Entry>
class LRU
{
public:
	bool empty() const
	{
		return lru.empty();
	}

	void add(const Key& key, const Entry& entry)
	{
		lru.push_back(KeyAndEntry(key, entry));
	}

	bool find(const Key& key, const Entry** pentry) const
	{
		CIt it = std::find(lru.begin(), lru.end(), KeyAndEntry(key));
		if(it == lru.end())
			return false;
		*pentry = &it->entry;
		return true;
	}

	void remove(const Key& key)
	{
		lru.remove(KeyAndEntry(key));
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
		DEBUG_WARN_ERR(ERR::LOGIC);	// entry not found in list
	}

	void remove_least_valuable(std::list<Entry>& entry_list)
	{
		entry_list.push_back(lru.front().entry);
		lru.pop_front();
	}

private:
	struct KeyAndEntry
	{
		KeyAndEntry(const Key& key): key(key) {}
		KeyAndEntry(const Key& key, const Entry& entry): key(key), entry(entry) {}

		bool operator==(const KeyAndEntry& rhs) const { return key == rhs.key; }
		bool operator!=(const KeyAndEntry& rhs) const { return !operator==(rhs); }

		Key key;
		Entry entry;
	};

	typedef std::list<KeyAndEntry> List;
	typedef typename List::iterator It;
	typedef typename List::const_iterator CIt;
	List lru;
};


// this is applicable to all cache management policies and stores all
// required information. a Divider functor is used to implement
// division for credit_density.
template<class Item, class Divider> struct CacheEntry
{
	Item item;
	size_t size;
	size_t cost;
	float credit;

	Divider divider;

	// needed for mgr.remove_least_valuable's entry_copy
	CacheEntry()
	{
	}

	CacheEntry(const Item& item_, size_t size_, size_t cost_)
		: item(item_), divider((float)size_)
	{
		size = size_;
		cost = cost_;
		credit = (float)cost;

		// else divider will fail
		ENSURE(size != 0);
	}

	float credit_density() const
	{
		return divider(credit, (float)size);
	}
};


//
// Cache
//

template
<
typename Key, typename Item,
// see documentation above for Manager's interface.
template<typename Key_, class Entry> class Manager = Landlord_Cached,
class Divider = Divider_Naive
>
class Cache
{
public:
	Cache() : mgr() {}

	void add(const Key& key, const Item& item, size_t size, size_t cost)
	{
		return mgr.add(key, Entry(item, size, cost));
	}

	// remove the entry identified by <key>. expected usage is to check
	// if present and determine size via retrieve(), so no need for
	// error checking.
	// useful for invalidating single cache entries.
	void remove(const Key& key)
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
	bool retrieve(const Key& key, Item& item, size_t* psize = 0, bool refill_credit = true)
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

	bool peek(const Key& key, Item& item, size_t* psize = 0)
	{
		return retrieve(key, item, psize, false);
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
			ENSURE(!entries_awaiting_eviction.empty());
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
	typedef CacheEntry<Item, Divider> Entry;

	// see note in remove_least_valuable().
	std::list<Entry> entries_awaiting_eviction;

	Manager<Key, Entry> mgr;
};

#endif	// #ifndef INCLUDED_CACHE_ADT
