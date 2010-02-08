/* Copyright (c) 2010 Wildfire Games
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
 * shared storage for strings
 */

#ifndef INCLUDED_STRING_POOL
#define INCLUDED_STRING_POOL

#include "lib/adts.h"	// DynHashTbl
#include "pool.h"

class StringPool
{
public:
	StringPool(size_t maxSize);
	~StringPool();

	/**
	 * allocate a copy of the string.
	 *
	 * @return a unique pointer for the string (addresses are equal iff
	 * the string contents match). can return 0, but would raise a
	 * warning first.
	 **/
	const char* UniqueCopy(const char* string);

	bool Contains(const char* string) const;

	const char* RandomString() const;

private:
	// rationale: we want an O(1) Contains() so that redundant UniqueCopy
	// calls are cheap. that requires allocating from one contiguous arena,
	// which is also more memory-efficient than the heap (no headers).
	Pool m_pool;

	typedef DynHashTbl<const char*, const char*> Map;
	Map m_map;
};

#endif	// #ifndef INCLUDED_STRING_POOL
