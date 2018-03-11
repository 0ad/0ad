/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_X86_X64_CACHE
#define INCLUDED_X86_X64_CACHE

namespace x86_x64 {

struct Cache	// POD (may be used before static constructors)
{
	enum Type
	{
		// (values match the CPUID.4 definition)
		kNull,
		kData,
		kInstruction,
		kUnified
		// note: further values are "reserved"
	};

	static const size_t maxLevels = 4;

	static const size_t fullyAssociative = 0xFF;	// (CPUID.4 definition)

	/**
	 * 1..maxLevels
	 **/
	size_t m_Level;

	/**
	 * never kNull
	 **/
	Type m_Type;

	/**
	 * if 0, the cache is disabled and all other values are zero
	 **/
	size_t m_NumEntries;

	/**
	 * NB: cache entries are lines, TLB entries are pages
	 **/
	size_t m_EntrySize;

	/**
	 * = fullyAssociative or the actual ways of m_Associativity
	 **/
	size_t m_Associativity;

	/**
	 * how many logical processors share this cache?
	 **/
	size_t m_SharedBy;

	void Initialize(size_t level, Type type)
	{
		m_Level   = level;
		m_Type    = type;
		m_NumEntries    = 0;
		m_EntrySize     = 0;
		m_Associativity = 0;
		m_SharedBy      = 0;

		ENSURE(Validate());
	}

	bool Validate() const
	{
		if(!(1 <= m_Level && m_Level <= maxLevels))
			return false;

		if(m_Type == kNull)
			return false;

		if(m_NumEntries == 0)	// disabled
		{
			if(m_EntrySize != 0)
				return false;
			if(m_Associativity != 0)
				return false;
			if(m_SharedBy != 0)
				return false;
		}
		else
		{
			if(m_EntrySize == 0)
				return false;
			if(m_Associativity == 0 || m_Associativity > fullyAssociative)
				return false;
			if(m_SharedBy == 0)
				return false;
		}

		return true;
	}

	u64 TotalSize() const
	{
		return u64(m_NumEntries)*m_EntrySize;
	}
};

enum IdxCache
{
	// (AddCache relies upon this order)
	L1D = 1,
	L2D,
	L3D,
	L4D,
	L1I,
	L2I,
	L3I,
	L4I,
	TLB
};

/**
 * @return 0 if idxCache >= TLB+numTLBs, otherwise a valid pointer to
 * a Cache whose m_NumEntries is 0 if disabled / not present.
 **/
LIB_API const Cache* Caches(size_t idxCache);

}	// namespace x86_x64

#endif	// #ifndef INCLUDED_X86_X64_CACHE
