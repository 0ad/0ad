#include "lib.h"

#include <list>
#include <map>



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
		const_iterator() : data(0), pos(0) {}
		const_iterator(const T* _data, size_t _pos) : data(_data), pos(_pos) {}
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

class Cache
{
public:
	// return the pointer associated with the line identified by tag,
	// or 0 if not in cache.
	void* get(u64 tag);

	// add tag to cache, and associate p with it.
	// return 0 on success, or -1 if already in cache.
	int add(u64 tag, void* p);

	// find least recently used entry that isn't locked;
	// change its tag, and return its associated pointer.
	void* replace_lru_with(u64 new_tag);

	int get_ctx(u64 tag, uintptr_t& ctx)
	{
		Line* l = find_line(tag);
		if(!l)
			return -1;
		ctx = l->ctx;
		return 0;
	}

	int set_ctx(u64 tag, uintptr_t ctx)
	{
		Line* l = find_line(tag);
		if(!l)
			return -1;
		l->ctx = ctx;
		return 0;
	}

	int lock(u64 tag, bool locked)
	{
		Line* l = find_line(tag);
		if(!l)
			return -1;
		l->locked = locked;
		return 0;
	}

private:
	// implementation:
	// cache lines are stored in a list, most recently used in front.
	// a map finds the list entry containing a given tag in log-time.

	struct Line
	{
		u64 tag;
		void* p;
		bool locked;	// protect from displacement
		uintptr_t ctx;

		Line(u64 _tag, void* _p)
		{
			tag = _tag;
			p = _p;
			locked = false;
			ctx = 0;
		}
	};

	typedef std::list<Line> List;
	typedef List::iterator List_iterator;
	List lru_list;

	typedef std::map<u64, List_iterator> Map;
	Map idx;


	// return the line identified by tag, or 0 if not in cache.
	Line* find_line(u64 tag);

	// mark l as the most recently used line.
	void reference(List_iterator l);
};
