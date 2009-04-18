/**
 * =========================================================================
 * File        : string_pool.h
 * Project     : 0 A.D.
 * Description : shared storage for strings
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
