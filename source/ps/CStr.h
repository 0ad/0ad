/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * File        : CStr.h
 * Project     : engine
 * Description : Contains CStr class which is a versatile class for making string use easy.
 *			   : The class implements a series of string manipulation/formatting functions.
 **/
/*
Examples:
	Generic text mapping is simply the ability to compile the final game in both UNICODE and ANSI.
	These are the ways in which we deal with strings.
	UNICODE uses 16 bits per character, whereas ANSI uses the traditional 8 bit character.
	In order to use both, most of the standard library functions come in T form.
	The following shows several examples of traditional ANSI vs UNICODE.
		// ANSI
		LPCSTR str = "M_PI";
		printf( "%s = %fn", str, 3.1459f );

		// UNICODE
		LPCWSTR str = L"M_PI";
		wprintf( L"%ls = %fn", str, 3.1459f );
*/

// history:
// 2004-05-19 Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)
// 2004-06-18 janwas: replaced conversion buffer with stringstream
// 2004-10-31 Philip: Changed to inherit from std::[w]string
// 2007-01-26 greybeard(joe@wildfiregames.com): added comments for doc generation

#ifndef INCLUDED_CSTR
#define INCLUDED_CSTR

/**
 * Whitespace trim identifier for Trim and Pad functions
 **/
enum PS_TRIM_MODE
{
	PS_TRIM_LEFT,	/// Trim all white space from the beginning of the string
	PS_TRIM_RIGHT,	/// Trim all white space from the end of the string
	PS_TRIM_BOTH	/// Trim all white space from the beginning and end of the string
};

#ifndef IN_UNIDOUBLER
 #define UNIDOUBLER_HEADER "CStr.h"
 #include "UniDoubler.h"
#endif

#endif

// Include this section when in unidoubler mode, and when this unicode/ascii
// version has not already been included.
#if defined(IN_UNIDOUBLER) && ( (defined(_UNICODE) && !defined(CSTR_H_U)) || (!defined(_UNICODE) && !defined(CSTR_H_A)) )

#ifdef _UNICODE
#define CSTR_H_U
#else
#define CSTR_H_A
#endif

#include <string>
#include "ps/utf16string.h"

class CStr8;
class CStrW;

/**
 * The base class of all strings
 **/
class CStr: public std::tstring
{
	// The two variations must be friends with each other
#ifdef _UNICODE
	friend class CStr8;
#else
	friend class CStrW;
#endif

public:

	// CONSTRUCTORS

	/**
	 * Constructor
	 *
	 **/
	CStr() {}
	/**
	 * Alternate Constructor
	 *
	 * @param const CStr & String reference to another CStr object to be used for initialization
	 **/
	CStr(const CStr& String) : std::tstring(String) {}
	/**
	 * Alternate Constructor
	 *
	 * @param const tchar * String pointer to an array of tchar to be used for initialization
	 **/
	CStr(const tchar* String) : std::tstring(String) {}
	/**
	 * Alternate Constructor
	 *
	 * @param const tchar * String pointer to first tchar to be used for initialization
	 * @param size_t Length number of tchar to be used from first tchar
	 **/
	CStr(const tchar* String, size_t Length) : std::tstring(String, Length) {}
	/**
	 * Alternate Constructor
	 *
	 * @param const tchar Char tchar to be used for initialization
	 **/
	CStr(tchar Char) : std::tstring(1, Char) {} // std::string's constructor is (repeats, chr)
	/**
	 * Alternate Constructor
	 *
	 * @param std::tstring String reference to basic string object to be used for inititalization
	 **/
	CStr(const std::tstring& String) : std::tstring(String) {}

	/**
	 * Repeat: Named constructor, to avoid overload overload.
	 *
	 * @param const CStr & String reference to another CStr object to be repeated for initialization
	 * @param size_t Reps number of times to repeat the initialization
	 * @return CStr new CStr object
	 **/
	static CStr Repeat(const CStr& String, size_t Reps);

	/**
	 * Construction from utf16strings.
	 * allowed on MSVC as of 2006-02-03 because utf16string is
	 * now distinct from CStrW.
	 *
	 * @param utf16string String utf16string to be used for initialization.
	 **/
	CStr(const utf16string& String) : std::tstring(String.begin(), String.end()) {}

	// Transparent CStrW/8 conversion. Non-ASCII characters are not
	// handled correctly.
	#ifndef _UNICODE
		CStr8(const CStrW& wideStr);
	#else
		CStrW(const CStr8& asciiStr);
	#endif

	// Conversion to/from UTF-8, encoded in a CStr8.
	// Invalid bytes/characters (e.g. broken UTF-8, and Unicode characters
	// above U+FFFF) are silently replaced with U+FFFD.
	#ifdef _UNICODE
		CStr8 ToUTF8() const;
	#else
		CStrW FromUTF8() const;
	#endif

	/**
	 * Alternate Constructor
	 *
	 * @param int Number integer to be used for initialization
	 **/
	CStr(int Number);
	/**
	 * Alternate Constructor
	 *
	 * @param unsigned int Number unsigned integer to be used for initialization
	 **/
	CStr(unsigned int Number);
	/**
	 * Alternate Constructor
	 *
	 * @param long Number long to be used for initialization
	 **/
	CStr(long Number);
	/**
	 * Alternate Constructor
	 *
	 * @param unsigned long Number unsigned long to be used for initialization
	 **/
	CStr(unsigned long Number);
	/**
	 * Alternate Constructor
	 *
	 * @param float Number float to be used for initialization
	 **/
	CStr(float Number);
	/**
	 * Alternate Constructor
	 *
	 * @param double Number double to be used for initialization
	 **/
	CStr(double Number);

	/**
	 * Destructor
	 *
	 **/
	//~CStr() {};

	// Conversions:

	/**
	 * Return CStr as Integer.
	 * Conversion is from the beginning of CStr.
	 *
	 * @return int CStr represented as an integer.
	 **/
	int ToInt() const;
	/**
	 * Return CStr as Unsigned Integer.
	 * Conversion is from the beginning of CStr.
	 *
	 * @return unsigned int CStr represented as an unsigned integer.
	 **/
	unsigned int ToUInt() const;
	/**
	 * Return CStr as Long.
	 * Conversion is from the beginning of CStr.
	 *
	 * @return long CStr represented as a long.
	 **/
	long ToLong() const;
	/**
	 * Return CStr as Unsigned Long.
	 * Conversion is from the beginning of CStr.
	 *
	 * @return unsigned long CStr represented as an unsigned long.
	 **/
	unsigned long ToULong() const;
	/**
	 * Return CStr as Float.
	 * Conversion is from the beginning of CStr.
	 *
	 * @return float CStr represented as a float.
	 **/
	float ToFloat() const;
	/**
	 * Return CStr as Double.
	 * Conversion is from the beginning of CStr.
	 *
	 * @return double CStr represented as a double.
	 **/
	double ToDouble() const;

	/**
	 * Search the CStr for another string.
	 * The search is case-sensitive.
	 *
	 * @param const CStr & Str reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long Find(const CStr& Str) const;
	/**
	 * Search the CStr for another string.
	 * The search is case-sensitive.
	 *
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long Find(const tchar chr) const;
	/**
	 * Search the CStr for another string with starting offset.
	 * The search is case-sensitive.
	 *
	 * @param const int & start character offset into CStr to begin search
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long Find(const int start, const tchar chr) const;

	/**
	 * Search the CStr for another string.
	 * The search is case-insensitive.
	 *
	 * @param const CStr & Str reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long FindInsensitive(const CStr& Str) const;
	/**
	 * Search the CStr for another string.
	 * The search is case-insensitive.
	 *
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long FindInsensitive(const tchar chr) const;
	/**
	 * Search the CStr for another string with starting offset.
	 * The search is case-insensitive.
	 *
	 * @param const int & start character offset into CStr to begin search
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long FindInsensitive(const int start, const tchar chr) const;

	/**
	 * Search the CStr for another string.
	 * The search is case-sensitive.
	 *
	 * @param const CStr & Str reference to the search string
	 * @return long offset into the CStr of the last occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long ReverseFind(const CStr& Str) const;

	/**
	 * Make a copy of the CStr in lower-case.
	 *
	 * @return CStr converted copy of CStr.
	 **/
	CStr LowerCase() const;
	/**
	 * Make a copy of the CStr in upper-case.
	 *
	 * @return CStr converted copy of CStr.
	 **/
	CStr UpperCase() const;

	/**
	 * Retrieve first n characters of the CStr.
	 *
	 * @param size_t len the number of characters to retrieve.
	 * @return CStr retrieved substring.
	 **/
	CStr Left(size_t len) const;

	/**
	 * Retrieve last n characters of the CStr.
	 *
	 * @param size_t len the number of characters to retrieve.
	 * @return CStr retrieved substring.
	 **/
	CStr Right(size_t len) const;

	/**
	 * Retrieve substring of the CStr after last occurrence of a string.
	 * Return substring of the CStr after the last occurrence of the search string.
	 *
	 * @param const CStr & Str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring remaining after match
	 *					 the CStr if no match is found
	 **/
	CStr AfterLast(const CStr& Str, size_t startPos = npos) const;

	/**
	 * Retrieve substring of the CStr preceding last occurrence of a string.
	 * Return substring of the CStr preceding the last occurrence of the search string.
	 *
	 * @param const CStr & Str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring preceding before match
	 *					 the CStr if no match is found
	 **/
	CStr BeforeLast(const CStr& Str, size_t startPos = npos) const;

	/**
	 * Retrieve substring of the CStr after first occurrence of a string.
	 * Return substring of the CStr after the first occurrence of the search string.
	 *
	 * @param const CStr & Str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring remaining after match
	 *					 the CStr if no match is found
	 **/
	CStr AfterFirst(const CStr& Str, size_t startPos = 0) const;

	/**
	 * Retrieve substring of the CStr preceding first occurrence of a string.
	 * Return substring of the CStr preceding the first occurrence of the search string.
	 *
	 * @param const CStr & Str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring preceding before match
	 *					 the CStr if no match is found
	 **/
	CStr BeforeFirst(const CStr& Str, size_t startPos = 0) const;

	/**
	 * Remove all occurrences of a string from the CStr.
	 *
	 * @param const CStr & Str reference to search string to remove.
	 **/
	void Remove(const CStr& Str);

	/**
	 * Replace all occurrences of one string by another string in the CStr.
	 *
	 * @param const CStr & StrToReplace reference to search string.
	 * @param const CStr & ReplaceWith reference to replace string.
	 **/
	void Replace(const CStr& StrToReplace, const CStr& ReplaceWith);

	/**
	 * Convert strings like  \\\n  into <backslash><newline>
	 *
	 * @return CStr converted copy of CStr.
	 **/
	CStr UnescapeBackslashes() const;

	/**
	 * Return a trimmed copy of the CStr.
	 *
	 * @param PS_TRIM_MODE Mode value from trim mode enumeration.
	 * @return CStr copy of trimmed CStr.
	 **/
	CStr Trim(PS_TRIM_MODE Mode) const;

	/**
	 * Return a space padded copy of the CStr.
	 *
	 * @param PS_TRIM_MODE Mode value from trim mode enumeration.
	 * @param size_t Length number of pad spaces to add
	 * @return CStr copy of padded CStr.
	 **/
	CStr Pad(PS_TRIM_MODE Mode, size_t Length) const;

	// Overloaded operations

	/**
	 * Set the CStr equal to an integer.
	 *
	 * @param int Number integer to convert to a CStr.
	 * @return CStr
	 **/
	CStr& operator=(int Number);
	/**
	 * Set the CStr equal to a long.
	 *
	 * @param long Number long to convert to a CStr.
	 * @return CStr
	 **/
	CStr& operator=(long Number);
	/**
	 * Set the CStr equal to an unsigned integer.
	 *
	 * @param unsigned int Number unsigned int to convert to a CStr.
	 * @return CStr
	 **/
	CStr& operator=(unsigned int Number);
	/**
	 * Set the CStr equal to an unsigned long.
	 *
	 * @param unsigned long Number unsigned long to convert to a CStr.
	 * @return CStr
	 **/
	CStr& operator=(unsigned long Number);
	/**
	 * Set the CStr equal to a float.
	 *
	 * @param float Number float to convert to a CStr.
	 * @return CStr
	 **/
	CStr& operator=(float Number);
	/**
	 * Set the CStr equal to a double.
	 *
	 * @param double Number double to convert to a CStr.
	 * @return CStr
	 **/
	CStr& operator=(double Number);

	CStr  operator+(const CStr& Str);
	CStr  operator+(const tchar* Str);
#ifndef _UNICODE
	CStr8 operator+(const CStrW& Str);
#else
	CStrW operator+(const CStr8& Str);
#endif

	operator const tchar*() const;

	tchar& operator[](size_t n)	{ return this->std::tstring::operator[](n); }
	tchar& operator[](int n)	{ return this->std::tstring::operator[](n); }

	// Conversion to utf16string
	utf16string utf16() const { return utf16string(begin(), end()); }

	// Calculates a hash of the string's contents
	size_t GetHashCode() const;

	// Serialization functions
	// (These are not virtual or inherited from ISerializable, to avoid
	// adding a vtable and making the strings larger than std::string)
	size_t GetSerializedLength() const;
	u8* Serialize(u8* buffer) const;
	const u8* Deserialize(const u8* buffer, const u8* bufferend);
};

// Hash function (for STL_HASH_MAP, etc)
class CStr_hash_compare
{
public:
	static const size_t bucket_size = 1;
	static const size_t min_buckets = 16;
	size_t operator() (const CStr& Key) const
	{
		return Key.GetHashCode();
	}
	bool operator() (const CStr& _Key1, const CStr& _Key2) const
	{
		return (_Key1 < _Key2);
	}
};

#endif
