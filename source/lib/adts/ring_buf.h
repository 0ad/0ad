/* Copyright (C) 2011 Wildfire Games.
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
 * static array, accessible modulo n
 */

#ifndef INCLUDED_ADTS_RING_BUF
#define INCLUDED_ADTS_RING_BUF

template<class T, size_t n> class RingBuf
{
	size_t size_;	// # of entries in buffer
	size_t head;	// index of oldest item
	size_t tail;	// index of newest item
	T data[n];

public:
	RingBuf() : data() { clear(); }
	void clear() { size_ = 0; head = 0; tail = n-1; }

	size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }

	const T& operator[](int ofs) const
	{
		ENSURE(!empty());
		size_t idx = (size_t)(head + ofs);
		return data[idx % n];
	}
	T& operator[](int ofs)
	{
		ENSURE(!empty());
		size_t idx = (size_t)(head + ofs);
		return data[idx % n];
	}

	T& front()
	{
		ENSURE(!empty());
		return data[head];
	}
	const T& front() const
	{
		ENSURE(!empty());
		return data[head];
	}
	T& back()
	{
		ENSURE(!empty());
		return data[tail];
	}
	const T& back() const
	{
		ENSURE(!empty());
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
			DEBUG_WARN_ERR(ERR::LOGIC);	// underflow
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
		return iterator(data, head);
	}
	const_iterator begin() const
	{
		return const_iterator(data, head);
	}
	iterator end()
	{
		return iterator(data, head+size_);
	}
	const_iterator end() const
	{
		return const_iterator(data, head+size_);
	}
};

#endif	// #ifndef INCLUDED_ADTS_RING_BUF
