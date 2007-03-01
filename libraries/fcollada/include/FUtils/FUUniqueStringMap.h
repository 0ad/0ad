/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUUniqueStringMap.h
	This file contains the FUUniqueStringMapT template.
*/

#ifndef _FU_UNIQUE_ID_MAP_H_
#define _FU_UNIQUE_ID_MAP_H_

/**
	A set of unique strings.
	This class adds three functions to the STL map in order to
	keep the strings inside unique: AddUniqueString, Exists and Erase.
	
	@ingroup FUtils
*/
template <typename STRING_BUILDER>
class FUUniqueStringMapT : fm::map<typename STRING_BUILDER::String, void*>
{
private:
	typedef fm::map<typename STRING_BUILDER::String, void*> Parent;
	typedef typename std::map<typename STRING_BUILDER::String, void*>::iterator iterator;
	typedef typename std::map<typename STRING_BUILDER::String, void*>::const_iterator const_iterator;
	
	const_iterator end() const { return Parent::end(); }

public:
	/** Adds a string to the map.
		If the string isn't unique, it will be modified in order to make it unique.
		@param wantedStr The string to add. This reference will be directly
			modified to hold the actual unique string added to the map. */
	void AddUniqueString(typename STRING_BUILDER::String& wantedStr);

	/** Retrieves whether a given string is contained within the map.
		@param str The string. */
	bool Exists(const typename STRING_BUILDER::String& str) const;

	/** Erases a string from the map.
		@param str A string contained within the map. */
	void Erase(const typename STRING_BUILDER::String& str);
};

typedef FUUniqueStringMapT<FUSStringBuilder> FUSUniqueStringMap; /**< A map of unique UTF-8 strings. */
typedef FUUniqueStringMapT<FUStringBuilder> FUUniqueStringMap; /**< A map of unique Unicode strings. */

template <typename Builder>
void FUUniqueStringMapT<Builder>::AddUniqueString(typename Builder::String& wantedStr)
{
	if (Exists(wantedStr))
	{
		// First, extract any existing number at the end of the name.
		size_t numberPosition = 1;
		size_t numberValue = 0;
		size_t c = wantedStr[wantedStr.length() - 1];
		while (c >= '0' && c <= '9')
		{
			numberValue += (((uint32) c) - '0') * numberPosition;
			wantedStr.erase(wantedStr.length() - 1);
			numberPosition *= 10;
			c = wantedStr[wantedStr.length() - 1];
		}
		
		// Attempt to generate a new string by appending an increasing counter.
		size_t counter = max(numberValue, (size_t) 2);
		Builder buffer;
		buffer.reserve(wantedStr.length() + 8);
		static const size_t maxCounter = 256;
		do
		{
			buffer.set(wantedStr);
			buffer.append(counter);
		} while (counter++ < maxCounter && Exists(buffer.ToString()));
	    
		// Hopefully, this is now unique.
		wantedStr = buffer.ToString();
	}
	insert(typename Parent::value_type(wantedStr, NULL));
}

template <typename Builder>
bool FUUniqueStringMapT<Builder>::Exists(const typename Builder::String& str) const
{
	const_iterator it = find(str);
	return it != end();
}

template <typename Builder>
void FUUniqueStringMapT<Builder>::Erase(const typename Builder::String& str)
{
	iterator it = find(str);
	if (it != end()) erase(it);
}

#endif // _FU_UNIQUE_ID_MAP_H_

