/* Copyright (C) 2022 Wildfire Games.
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
 * Description : Contains CStr class which is a versatile class for making string use easy.
 *			   : The class implements a series of string manipulation/formatting functions.
 **/

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

#include "ps/CStrForward.h"

#include <string>

/**
 * The base class of all strings
 **/
class CStr : public std::tstring
{
public:
	using StrBase = std::tstring;
	using Char = typename std::tstring::value_type;

	CStr(const StrBase& str) : StrBase(str) {}

	using StrBase::StrBase;

	/**
	 * Repeat: Named constructor, to avoid overload overload.
	 *
	 * @param const CStr & str reference to another CStr object to be repeated for initialization
	 * @param size_t Reps number of times to repeat the initialization
	 * @return CStr new CStr object
	 **/
	static CStr Repeat(const CStr& str, size_t reps);

	/**
	 * Construction from u16strings.
	 *
	 * @param u16string String u16string to be used for initialization.
	 **/
	explicit CStr(const std::u16string& str) : StrBase(str.begin(), str.end()) {}

	// Conversion to/from UTF-8, encoded in a CStr8.
	// Invalid bytes/characters (e.g. broken UTF-8, and Unicode characters
	// above U+FFFF) are silently replaced with U+FFFD.
	#ifdef _UNICODE
		CStr8 ToUTF8() const;
	#else
		CStrW FromUTF8() const;
	#endif

	// Conversions:

	static CStr FromInt(int n);
	static CStr FromUInt(unsigned int n);
	static CStr FromInt64(i64 n);
	static CStr FromDouble(double n);

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
	 * @param const CStr & str reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long Find(const CStr& str) const;
	/**
	 * Search the CStr for another string.
	 * The search is case-sensitive.
	 *
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long Find(const Char chr) const;
	/**
	 * Search the CStr for another string with starting offset.
	 * The search is case-sensitive.
	 *
	 * @param const int & start character offset into CStr to begin search
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long Find(const int start, const Char chr) const;

	/**
	 * Search the CStr for another string.
	 * The search is case-insensitive.
	 *
	 * @param const CStr & str reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long FindInsensitive(const CStr& str) const;
	/**
	 * Search the CStr for another string.
	 * The search is case-insensitive.
	 *
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long FindInsensitive(const Char chr) const;
	/**
	 * Search the CStr for another string with starting offset.
	 * The search is case-insensitive.
	 *
	 * @param const int & start character offset into CStr to begin search
	 * @param const {t|w}char_t & chr reference to the search string
	 * @return long offset into the CStr of the first occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long FindInsensitive(const int start, const Char chr) const;

	/**
	 * Search the CStr for another string.
	 * The search is case-sensitive.
	 *
	 * @param const CStr & str reference to the search string
	 * @return long offset into the CStr of the last occurrence of the search string
	 *				-1 if the search string is not found
	 **/
	long ReverseFind(const CStr& str) const;

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
	 * @param const CStr & str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring remaining after match
	 *					 the CStr if no match is found
	 **/
	CStr AfterLast(const CStr& str, size_t startPos = npos) const;

	/**
	 * Retrieve substring of the CStr preceding last occurrence of a string.
	 * Return substring of the CStr preceding the last occurrence of the search string.
	 *
	 * @param const CStr & str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring preceding before match
	 *					 the CStr if no match is found
	 **/
	CStr BeforeLast(const CStr& str, size_t startPos = npos) const;

	/**
	 * Retrieve substring of the CStr after first occurrence of a string.
	 * Return substring of the CStr after the first occurrence of the search string.
	 *
	 * @param const CStr & str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring remaining after match
	 *					 the CStr if no match is found
	 **/
	CStr AfterFirst(const CStr& str, size_t startPos = 0) const;

	/**
	 * Retrieve substring of the CStr preceding first occurrence of a string.
	 * Return substring of the CStr preceding the first occurrence of the search string.
	 *
	 * @param const CStr & str reference to search string
	 * @param size_t startPos character position to start searching from
	 * @return CStr substring preceding before match
	 *					 the CStr if no match is found
	 **/
	CStr BeforeFirst(const CStr& str, size_t startPos = 0) const;

	/**
	 * Remove all occurrences of a string from the CStr.
	 *
	 * @param const CStr & str reference to search string to remove.
	 **/
	void Remove(const CStr& str);

	/**
	 * Replace all occurrences of one string by another string in the CStr.
	 *
	 * @param const CStr & strToReplace reference to search string.
	 * @param const CStr & replaceWith reference to replace string.
	 **/
	void Replace(const CStr& toReplace, const CStr& replaceWith);

	/**
	 * Convert strings to printable ASCII characters with JSON-style escapes.
	 */
	std::string EscapeToPrintableASCII() const;

	/**
	 * Return a trimmed copy of the CStr.
	 *
	 * @param PS_TRIM_MODE Mode value from trim mode enumeration.
	 * @return CStr copy of trimmed CStr.
	 **/
	CStr Trim(PS_TRIM_MODE mode) const;

	/**
	 * Return a space padded copy of the CStr.
	 *
	 * @param PS_TRIM_MODE Mode value from trim mode enumeration.
	 * @param size_t Length number of pad spaces to add
	 * @return CStr copy of padded CStr.
	 **/
	CStr Pad(PS_TRIM_MODE mode, size_t len) const;

	// Conversion to u16string
	std::u16string utf16() const { return std::u16string(begin(), end()); }

	// Calculates a hash of the string's contents
	size_t GetHashCode() const;

	// Serialization functions
	// (These are not virtual or inherited from ISerializable, to avoid
	// adding a vtable and making the strings larger than std::string)
	size_t GetSerializedLength() const;
	u8* Serialize(u8* buffer) const;
	const u8* Deserialize(const u8* buffer, const u8* bufferend);
};

namespace std
{
template <>
struct hash<CStr>
{
	std::size_t operator()(const CStr& str) const
	{
		return str.GetHashCode();
	}
};
}

#endif // INCLUDED_CSTR
