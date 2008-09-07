/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUStringBuilder.h"
#include "FUUniqueStringMap.h"

// Externed by FUStringConversion.cpp
void TrickLinkerFUUniqueStringMap()
{
	// Since the header file typedefs the ONLY two wanted unique-string-map types, these functions should be created
	// by the "exercising" functions.

	FUUniqueStringMap map1; FUSUniqueStringMap map2;
	fstring test1(FC("Test1")); fm::string test2("Test2");
	map1.insert(test1); map2.insert(test2);
	if (map1.contains(test1)) map1.erase(test1);
	if (map2.contains(test2)) map2.erase(test2);
}

template <class CH>
inline void SplitString(const fm::stringT<CH>& str, fm::stringT<CH>& prefix, uint32& suffix)
{
	// Remove the suffix from the string.
	size_t len = str.length();
	prefix = str;
	while (len > 0 && prefix[len-1] >= '0' && prefix[len-1] <= '9')
	{
		prefix.erase(len-1, len);
		--len;
	}

	// Parse the suffix: if no suffix is present, use -1.
	size_t offset = prefix.length();
	if (offset == str.length()) suffix = ~(uint32)0;
	else suffix = FUStringConversion::ToUInt32(str.c_str() + offset);
}

template <class CH>
void FUUniqueStringMapT<CH>::insert(fm::stringT<CH>& wantedStr)
{
	fm::stringT<CH> prefix; uint32 suffix;
	SplitString(wantedStr, prefix, suffix);

	typename StringMap::iterator itV = values.find(prefix);
	if (itV == values.end())
	{
		// Add this prefix to the string list.
		itV = values.insert(prefix, NumberMap());
	}

	NumberMap::iterator itN = itV->second.find(suffix);
	if (itN != itV->second.end())
	{
		// Retrieve the end of the tree.
		itN = itV->second.last();
		if (itN->first != ~(uint32)0) suffix = itN->first + 1;
		else
		{
			// Get the second last suffix, if it exists.
			--itN;
			if (itN == itV->second.end()) suffix = 2;
			suffix = itN->first + 1;
		}

		// Recreate the user's string to use this unique suffix.
		FUStringBuilderT<CH> builder(prefix);
		builder.append(suffix);
		wantedStr = builder.ToString();
	}

	// Insert our suffix in the known list.
	itV->second.insert(suffix, suffix);
}

template <class CH>
bool FUUniqueStringMapT<CH>::contains(const fm::stringT<CH>& str) const
{
	fm::stringT<CH> prefix; uint32 suffix;
	SplitString(str, prefix, suffix);

	typename StringMap::const_iterator itV = values.find(prefix);
	if (itV == values.end()) return false;
	else return itV->second.find(suffix) != itV->second.end();
}

template <class CH>
void FUUniqueStringMapT<CH>::erase(const fm::stringT<CH>& str)
{
	fm::stringT<CH> prefix; uint32 suffix;
	SplitString(str, prefix, suffix);

	typename StringMap::iterator itV = values.find(prefix);
	if (itV != values.end())
	{
		itV->second.erase(suffix);
	}
}
