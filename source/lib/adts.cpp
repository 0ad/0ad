#include "adts.h"

#include <cassert>

template<class T, int max_items> class DynArray
{
public:
	DynArray() {}
	~DynArray() {}

	T* operator[](uint n)
	{
		T*& page = pages[n / items_per_page];
		if(!page)
		{
			page = (T*)calloc(PAGE_SIZE, 1);
			if(!page)
				return 0;
		}
		return page + (n % items_per_page);
	}

private:
	enum { PAGE_SIZE = 4096 };

//	const size_t items_per_page = sizeof(T) / PAGE_SIZE;
//	const size_t num_pages = (max_items + items_per_page-1) / items_per_page;
	T* pages[(max_items + (sizeof(T) / PAGE_SIZE)-1) / (sizeof(T) / PAGE_SIZE)];
};




//
// cache implementation
//

// mark l as the most recently used line.
void Cache::reference(List_iterator l)
{
	lru_list.splice(lru_list.begin(), lru_list, l);
	idx[l->tag] = l;
}


// return the line identified by tag, or 0 if not in cache.
Cache::Line* Cache::find_line(u64 tag)
{
	Map::const_iterator i = idx.find(tag);
	// not found
	if(i == idx.end())
		return 0;

	// index points us to list entry
	List_iterator l = i->second;
	reference(l);
	return &*l;
}


// return the pointer associated with the line identified by tag,
// or 0 if not in cache.
void* Cache::get(u64 tag)
{
	Line* l = find_line(tag);
	return l? l->p : 0;
}


// add tag to cache, and associate p with it.
// return 0 on success, or -1 if already in cache.
int Cache::add(u64 tag, void* p)
{
	if(get(tag))
	{
		assert(0 && "add: tag already in cache!");
		return -1;
	}

	// add directly to front of LRU list
	lru_list.push_front(Line(tag, p));
	idx[tag] = lru_list.begin();
	return 0;
}


// find least recently used entry that isn't locked;
// change its tag, and return its associated pointer.
void* Cache::replace_lru_with(u64 new_tag)
{
	// scan in least->most used order for first non-locked entry
	List_iterator l = lru_list.end();
	while(l != lru_list.begin())
	{
		--l;
		if(!l->locked)
			goto have_entry;
	}

	// all are locked and cannot be displaced.
	// caller should add() enough lines so that this never happens.
	assert(0 && "replace_lru_with not possible - all lines are locked");
	return 0;

have_entry:

	idx.erase(l->tag);
	l->tag = new_tag;
	reference(l);
	return l->p;
}
