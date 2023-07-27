/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#ifndef CStr_CPP_FIRST
#define CStr_CPP_FIRST

#include "lib/fnv_hash.h"
#include "lib/utf8.h"
#include "lib/byte_order.h"
#include "network/Serialization.h"

#include <cctype>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace
{
	// Use a knowingly false expression, as we can't use
	// static_assert(false, ...) directly, because it's an ill-formed program
	// with a value (false) which doesn't depend on any input parameter.
	// We don't use constexpr bool AlwaysFalse = false, because some compilers
	// complain about an unused constant.
	template<typename T>
	struct AlwaysFalse : std::false_type
	{};

	template<typename StrBase>
	using tstringstream = std::basic_stringstream<typename StrBase::value_type>;

	template<typename Char>
	bool istspace(const Char chr)
	{
		if constexpr (std::is_same_v<Char, char>)
			return static_cast<bool>(std::isspace(chr));
		else
			return static_cast<bool>(std::iswspace(chr));
	}

	template<typename Char>
	Char totlower(const Char chr)
	{
		if constexpr (std::is_same_v<Char, char>)
			return std::tolower(chr);
		else
			return std::towlower(chr);
	}

	template<typename Char>
	Char totupper(const Char chr)
	{
		if constexpr (std::is_same_v<Char, char>)
			return std::toupper(chr);
		else
			return std::towupper(chr);
	}

	template<typename StrBase>
	u8* SerializeImpl(const StrBase& str, u8* buffer)
	{
		using Char = typename StrBase::value_type;
		ENSURE(buffer);
		if constexpr (std::is_same_v<Char, char>)
		{
			// CStr8 is always serialized to / from ASCII(or whatever 8 - bit codepage stored
			// in the CStr).
			size_t len = str.length();
			Serialize_int_4(buffer, (u32)len);
			size_t i = 0;
			for (i = 0; i < len; i++)
				buffer[i] = str[i];
			return buffer + len;
		}
		else if constexpr (std::is_same_v<Char, wchar_t>)
		{
			// CStrW is always serialized to / from UTF - 16.
			size_t len = str.length();
			size_t i = 0;
			for (i = 0; i < len; i++)
			{
				const u16 bigEndian = to_be16(str[i]);
				*(u16 *)(buffer + i * 2) = bigEndian;
			}
			*(u16 *)(buffer + i * 2) = 0;
			return buffer + len * 2 + 2;
		}
		else
			static_assert(AlwaysFalse<Char>::value, "Not implemented.");
	}

	template<typename StrBase>
	const u8* DeserializeImpl(const u8* buffer, const u8* bufferend, StrBase& str)
	{
		using Char = typename StrBase::value_type;
		ENSURE(buffer);
		ENSURE(bufferend);
		if constexpr (std::is_same_v<Char, char>)
		{
			u32 len;
			Deserialize_int_4(buffer, len);
			if (buffer + len > bufferend)
				return NULL;
			str = StrBase(buffer, buffer + len);
			return buffer + len;
		}
		else if constexpr (std::is_same_v<Char, wchar_t>)
		{
			const u16 *strend = (const u16 *)buffer;
			while ((const u8 *)strend < bufferend && *strend)
				strend++;
			if ((const u8 *)strend >= bufferend)
				return nullptr;

			str.resize(strend - (const u16 *)buffer);
			const u16 *ptr = (const u16 *)buffer;

			typename StrBase::iterator it = str.begin();
			while (ptr < strend)
			{
				const u16 native = to_be16(*(ptr++));	// we want from_be16, but that's the same
				*(it++) = (Char)native;
			}

			return (const u8 *)(strend + 1);
		}
		else
			static_assert(AlwaysFalse<Char>::value, "Not implemented.");
	}

	template<typename StrBase>
	size_t GetSerializedLengthImpl(const StrBase& str)
	{
		using Char = typename StrBase::value_type;
		if constexpr (std::is_same_v<Char, char>)
			return str.length() + 4;
		else if constexpr (std::is_same_v<Char, wchar_t>)
			return str.length() * 2 + 2;
		else
			static_assert(AlwaysFalse<Char>::value, "Not implemented.");
	}

} // anonymous namespace

#define UNIDOUBLER_HEADER "CStr.cpp"
#include "UniDoubler.h"


// Only include these function definitions in the first instance of CStr.cpp:

/**
 * Convert CStr to UTF-8
 *
 * @return CStr8 converted string
 **/
CStr8 CStrW::ToUTF8() const
{
	Status err;
	return utf8_from_wstring(*this, &err);
}

/**
 * Convert UTF-8 to CStr
 *
 * @return CStrW converted string
 **/
CStrW CStr8::FromUTF8() const
{
	Status err;
	return wstring_from_utf8(*this, &err);
}

#else

// The following code is compiled twice, as CStrW then as CStr8:

#include "CStr.h"

CStr CStr::Repeat(const CStr& str, size_t reps)
{
	CStr ret;
	ret.reserve(str.length() * reps);
	while (reps--) ret += str;
	return ret;
}

// Construction from numbers:

CStr CStr::FromInt(int n)
{
	tstringstream<StrBase> ss;
	ss << n;
	return ss.str();
}

CStr CStr::FromUInt(unsigned int n)
{
	tstringstream<StrBase> ss;
	ss << n;
	return ss.str();
}

CStr CStr::FromInt64(i64 n)
{
	tstringstream<StrBase> ss;
	ss << n;
	return ss.str();
}

CStr CStr::FromDouble(double n)
{
	tstringstream<StrBase> ss;
	ss << n;
	return ss.str();
}

// Conversion to numbers:

int CStr::ToInt() const
{
	int ret = 0;
	tstringstream<StrBase> str(*this);
	str >> ret;
	return ret;
}

unsigned int CStr::ToUInt() const
{
	unsigned int ret = 0;
	tstringstream<StrBase> str(*this);
	str >> ret;
	return ret;
}

long CStr::ToLong() const
{
	long ret = 0;
	tstringstream<StrBase> str(*this);
	str >> ret;
	return ret;
}

unsigned long CStr::ToULong() const
{
	unsigned long ret = 0;
	tstringstream<StrBase> str(*this);
	str >> ret;
	return ret;
}

/**
 * libc++ and libstd++ differ on how they handle string-to-number parsing for floating-points numbers.
 * See https://trac.wildfiregames.com/ticket/2780#comment:4 for details.
 * To prevent this, only consider [0-9.-+], replace the others in-place with a neutral character.
 */
CStr ParseableAsNumber(CStr cleaned_copy)
{
	for (CStr::Char& c : cleaned_copy)
		if (!std::isdigit(c) && c != '.' && c != '-' && c != '+')
			c = ' ';

	return cleaned_copy;
}

float CStr::ToFloat() const
{
	float ret = 0;
	tstringstream<StrBase> str(ParseableAsNumber(*this));
	str >> ret;
	return ret;
}

double CStr::ToDouble() const
{
	double ret = 0;
	tstringstream<StrBase> str(ParseableAsNumber(*this));
	str >> ret;
	return ret;
}

// Search the string for another string
long CStr::Find(const CStr& str) const
{
	size_t pos = find(str, 0);

	if (pos != npos)
		return static_cast<long>(pos);

	return -1;
}

// Search the string for another string
long CStr::Find(const Char chr) const
{
	size_t pos = find(chr, 0);

	if (pos != npos)
		return static_cast<long>(pos);

	return -1;
}

// Search the string for another string
long CStr::Find(const int start, const Char chr) const
{
	size_t pos = find(chr, start);

	if (pos != npos)
		return static_cast<long>(pos);

	return -1;
}

long CStr::FindInsensitive(const int start, const Char chr) const { return LowerCase().Find(start, totlower(chr)); }
long CStr::FindInsensitive(const Char chr) const { return LowerCase().Find(totlower(chr)); }
long CStr::FindInsensitive(const CStr& str) const { return LowerCase().Find(str.LowerCase()); }

long CStr::ReverseFind(const CStr& str) const
{
	size_t pos = rfind(str, length() );

	if (pos != npos)
		return static_cast<long>(pos);

	return -1;
}

// Lowercase and uppercase
CStr CStr::LowerCase() const
{
	StrBase newStr = *this;
	for (size_t i = 0; i < length(); i++)
		newStr[i] = (Char)totlower((*this)[i]);

	return newStr;
}

CStr CStr::UpperCase() const
{
	StrBase newStr = *this;
	for (size_t i = 0; i < length(); i++)
		newStr[i] = (Char)totupper((*this)[i]);

	return newStr;
}


// Retrieve the substring of the first n characters
CStr CStr::Left(size_t len) const
{
	ENSURE(len <= length());
	return substr(0, len);
}

// Retrieve the substring of the last n characters
CStr CStr::Right(size_t len) const
{
	ENSURE(len <= length());
	return substr(length()-len, len);
}

// Retrieve the substring following the last occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::AfterLast(const CStr& str, size_t startPos) const
{
	size_t pos = rfind(str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(pos + str.length());
}

// Retrieve the substring preceding the last occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::BeforeLast(const CStr& str, size_t startPos) const
{
	size_t pos = rfind(str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(0, pos);
}

// Retrieve the substring following the first occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::AfterFirst(const CStr& str, size_t startPos) const
{
	size_t pos = find(str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(pos + str.length());
}

// Retrieve the substring preceding the first occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::BeforeFirst(const CStr& str, size_t startPos) const
{
	size_t pos = find(str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(0, pos);
}

// Remove all occurrences of some character or substring
void CStr::Remove(const CStr& str)
{
	size_t foundAt = 0;
	while (foundAt != npos)
	{
		foundAt = find(str, 0);

		if (foundAt != npos)
			erase(foundAt, str.length());
	}
}

// Replace all occurrences of some substring by another
void CStr::Replace(const CStr& toReplace, const CStr& replaceWith)
{
	size_t pos = 0;
	while (pos != npos)
	{
		pos = find(toReplace, pos);
		if (pos != npos)
		{
			erase(pos, toReplace.length());
			insert(pos, replaceWith);
			pos += replaceWith.length();
		}
	}
}

std::string CStr::EscapeToPrintableASCII() const
{
	std::string newStr;
	for (size_t i = 0; i < length(); i++)
	{
		Char ch = (*this)[i];

		if (ch == '"') newStr += "\\\"";
		else if (ch == '\\') newStr += "\\\\";
		else if (ch == '\b') newStr += "\\b";
		else if (ch == '\f') newStr += "\\f";
		else if (ch == '\n') newStr += "\\n";
		else if (ch == '\r') newStr += "\\r";
		else if (ch == '\t') newStr += "\\t";
		else if (ch >= 32 && ch <= 126)
			newStr += ch;
		else
		{
			std::stringstream ss;
			ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)ch;
			newStr += ss.str();
		}
	}
	return newStr;
}

// Returns a trimmed string, removes whitespace from the left/right/both
CStr CStr::Trim(PS_TRIM_MODE mode) const
{
	size_t left = 0, right = 0;

	switch (mode)
	{
		case PS_TRIM_LEFT:
		{
			for (left = 0; left < length(); left++)
				if (istspace((*this)[left]) == false)
					break; // end found, trim 0 to Left-1 inclusive
		} break;

		case PS_TRIM_RIGHT:
		{
			right = length();
			while (right--)
				if (istspace((*this)[right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;

		case PS_TRIM_BOTH:
		{
			for (left = 0; left < length(); left++)
				if (istspace((*this)[left]) == false)
					break; // end found, trim 0 to Left-1 inclusive

			right = length();
			while (right--)
				if (istspace((*this)[right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;

		default:
			debug_warn(L"CStr::Trim: invalid Mode");
	}


	return substr(left, right - left + 1);
}

CStr CStr::Pad(PS_TRIM_MODE mode, size_t len) const
{
	size_t left = 0, right = 0;

	if (len <= length())
		return *this;

	// From here: Length-length() >= 1

	switch (mode)
	{
	case PS_TRIM_LEFT:
		left = len - length();
		break;

	case PS_TRIM_RIGHT:
		right = len - length();
		break;

	case PS_TRIM_BOTH:
		left = (len - length() + 1) / 2;
		right = (len - length() - 1) / 2; // cannot be negative
		break;

	default:
		debug_warn(L"CStr::Trim: invalid Mode");
	}

	return StrBase(left, ' ') + *this + StrBase(right, ' ');
}

size_t CStr::GetHashCode() const
{
	return (size_t)fnv_hash(data(), length()*sizeof(value_type));
		// janwas 2005-03-18: now use 32-bit version; 64 is slower and
		// the result was truncated down to 32 anyway.
}

u8* CStr::Serialize(u8* buffer) const
{
	return SerializeImpl(*this, buffer);
}

const u8* CStr::Deserialize(const u8* buffer, const u8* bufferend)
{
	return DeserializeImpl(buffer, bufferend, *this);
}

size_t CStr::GetSerializedLength() const
{
	return GetSerializedLengthImpl(*this);
}

#endif // CStr_CPP_FIRST
