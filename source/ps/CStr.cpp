/* Copyright (C) 2014 Wildfire Games.
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
 * File        : CStr.cpp
 * Project     : engine
 * Description : Controls compilation of CStr class and
 *             : includes some function implementations.
 **/
#include "precompiled.h"

#ifndef CStr_CPP_FIRST
#define CStr_CPP_FIRST

#include "lib/fnv_hash.h"
#include "lib/utf8.h"
#include "lib/byte_order.h"
#include "network/Serialization.h"

#include <iomanip>
#include <sstream>

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

#include <sstream>

#ifdef  _UNICODE
 #define tstringstream wstringstream
 #define _istspace iswspace
 #define _totlower towlower
 #define _totupper towupper
#else
 #define tstringstream stringstream
 #define _istspace isspace
 #define _totlower tolower
 #define _totupper toupper
#endif

CStr CStr::Repeat(const CStr& String, size_t Reps)
{
	CStr ret;
	ret.reserve(String.length() * Reps);
	while (Reps--) ret += String;
	return ret;
}

// Construction from numbers:

CStr CStr::FromInt(int n)
{
	std::tstringstream ss;
	ss << n;
	return ss.str();
}

CStr CStr::FromUInt(unsigned int n)
{
	std::tstringstream ss;
	ss << n;
	return ss.str();
}

CStr CStr::FromInt64(i64 n)
{
	std::tstringstream ss;
	ss << n;
	return ss.str();
}

CStr CStr::FromDouble(double n)
{
	std::tstringstream ss;
	ss << n;
	return ss.str();
}

// Conversion to numbers:

int CStr::ToInt() const
{
	int ret = 0;
	std::tstringstream str(*this);
	str >> ret;
	return ret;
}

unsigned int CStr::ToUInt() const
{
	unsigned int ret = 0;
	std::tstringstream str(*this);
	str >> ret;
	return ret;
}

long CStr::ToLong() const
{
	long ret = 0;
	std::tstringstream str(*this);
	str >> ret;
	return ret;
}

unsigned long CStr::ToULong() const
{
	unsigned long ret = 0;
	std::tstringstream str(*this);
	str >> ret;
	return ret;
}

float CStr::ToFloat() const
{
	float ret = 0;
	std::tstringstream str(*this);
	str >> ret;
	return ret;
}

double CStr::ToDouble() const
{
	double ret = 0;
	std::tstringstream str(*this);
	str >> ret;
	return ret;
}


// Search the string for another string
long CStr::Find(const CStr& Str) const
{
	size_t Pos = find(Str, 0);

	if (Pos != npos)
		return (long)Pos;

	return -1;
}

// Search the string for another string
long CStr::Find(const tchar chr) const
{
	size_t Pos = find(chr, 0);

	if (Pos != npos)
		return (long)Pos;

	return -1;
}

// Search the string for another string
long CStr::Find(const int start, const tchar chr) const
{
	size_t Pos = find(chr, start);

	if (Pos != npos)
		return (long)Pos;

	return -1;
}

long CStr::FindInsensitive(const int start, const tchar chr) const { return LowerCase().Find(start, _totlower(chr)); }
long CStr::FindInsensitive(const tchar chr) const { return LowerCase().Find(_totlower(chr)); }
long CStr::FindInsensitive(const CStr& Str) const { return LowerCase().Find(Str.LowerCase()); }


long CStr::ReverseFind(const CStr& Str) const
{
	size_t Pos = rfind(Str, length() );

	if (Pos != npos)
		return (long)Pos;

	return -1;

}

// Lowercase and uppercase
CStr CStr::LowerCase() const
{
	std::tstring NewString = *this;
	for (size_t i = 0; i < length(); i++)
		NewString[i] = (tchar)_totlower((*this)[i]);

	return NewString;
}

CStr CStr::UpperCase() const
{
	std::tstring NewString = *this;
	for (size_t i = 0; i < length(); i++)
		NewString[i] = (tchar)_totupper((*this)[i]);

	return NewString;
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
CStr CStr::AfterLast(const CStr& Str, size_t startPos) const
{
	size_t pos = rfind(Str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(pos + Str.length());
}

// Retrieve the substring preceding the last occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::BeforeLast(const CStr& Str, size_t startPos) const
{
	size_t pos = rfind(Str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(0, pos);
}

// Retrieve the substring following the first occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::AfterFirst(const CStr& Str, size_t startPos) const
{
	size_t pos = find(Str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(pos + Str.length());
}

// Retrieve the substring preceding the first occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::BeforeFirst(const CStr& Str, size_t startPos) const
{
	size_t pos = find(Str, startPos);
	if (pos == npos)
		return *this;
	else
		return substr(0, pos);
}

// Remove all occurrences of some character or substring
void CStr::Remove(const CStr& Str)
{
	size_t FoundAt = 0;
	while (FoundAt != npos)
	{
		FoundAt = find(Str, 0);
		
		if (FoundAt != npos)
			erase(FoundAt, Str.length());
	}
}

// Replace all occurrences of some substring by another
void CStr::Replace(const CStr& ToReplace, const CStr& ReplaceWith)
{
	size_t Pos = 0;
	
	while (Pos != npos)
	{
		Pos = find(ToReplace, Pos);
		if (Pos != npos)
		{
			erase(Pos, ToReplace.length());
			insert(Pos, ReplaceWith);
			Pos += ReplaceWith.length();
		}
	}
}

std::string CStr::EscapeToPrintableASCII() const
{
	std::string NewString;
	for (size_t i = 0; i < length(); i++)
	{
		tchar ch = (*this)[i];

		if (ch == '"') NewString += "\\\"";
		else if (ch == '\\') NewString += "\\\\";
		else if (ch == '\b') NewString += "\\b";
		else if (ch == '\f') NewString += "\\f";
		else if (ch == '\n') NewString += "\\n";
		else if (ch == '\r') NewString += "\\r";
		else if (ch == '\t') NewString += "\\t";
		else if (ch >= 32 && ch <= 126)
			NewString += ch;
		else
		{
			std::stringstream ss;
			ss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)ch;
			NewString += ss.str();
		}
	}
	return NewString;
}

// Returns a trimmed string, removes whitespace from the left/right/both
CStr CStr::Trim(PS_TRIM_MODE Mode) const
{
	size_t Left = 0, Right = 0;
	
	switch (Mode)
	{
		case PS_TRIM_LEFT:
		{
			for (Left = 0; Left < length(); Left++)
				if (_istspace((*this)[Left]) == false)
					break; // end found, trim 0 to Left-1 inclusive
		} break;
		
		case PS_TRIM_RIGHT:
		{
			Right = length();
			while (Right--)
				if (_istspace((*this)[Right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;
		
		case PS_TRIM_BOTH:
		{
			for (Left = 0; Left < length(); Left++)
				if (_istspace((*this)[Left]) == false)
					break; // end found, trim 0 to Left-1 inclusive

			Right = length();
			while (Right--)
				if (_istspace((*this)[Right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;

		default:
			debug_warn(L"CStr::Trim: invalid Mode");
	}


	return substr(Left, Right-Left+1);
}

CStr CStr::Pad(PS_TRIM_MODE Mode, size_t Length) const
{
	size_t Left = 0, Right = 0;

	if (Length <= length())
		return *this;
	
	// From here: Length-length() >= 1

	switch (Mode)
	{
	case PS_TRIM_LEFT:
		Left = Length - length();
		break;

	case PS_TRIM_RIGHT:
		Right = Length - length();
		break;

	case PS_TRIM_BOTH:
		Left  = (Length - length() + 1)/2;
		Right = (Length - length() - 1)/2; // cannot be negative
		break;

	default:
		debug_warn(L"CStr::Trim: invalid Mode");
	}

	return std::tstring(Left, _T(' ')) + *this + std::tstring(Right, _T(' '));
}

size_t CStr::GetHashCode() const
{
	return (size_t)fnv_hash(data(), length()*sizeof(value_type));
		// janwas 2005-03-18: now use 32-bit version; 64 is slower and
		// the result was truncated down to 32 anyway.
}

#ifdef _UNICODE
/*
	CStrW is always serialized to/from UTF-16
*/

u8* CStrW::Serialize(u8* buffer) const
{
	size_t len = length();
	size_t i = 0;
	for (i = 0; i < len; i++)
	{
		const u16 bigEndian = to_be16((*this)[i]);
		*(u16 *)(buffer + i*2) = bigEndian;
	}
	*(u16 *)(buffer + i*2) = 0;
	return buffer + len*2 + 2;
}

const u8* CStrW::Deserialize(const u8* buffer, const u8* bufferend)
{
	const u16 *strend = (const u16 *)buffer;
	while ((const u8 *)strend < bufferend && *strend) strend++;
	if ((const u8 *)strend >= bufferend) return NULL;

	resize(strend - (const u16 *)buffer);
	const u16 *ptr = (const u16 *)buffer;

	std::wstring::iterator str = begin();
	while (ptr < strend)
	{
		const u16 native = to_be16(*(ptr++));	// we want from_be16, but that's the same
		*(str++) = (tchar)native;
	}

	return (const u8 *)(strend+1);
}

size_t CStr::GetSerializedLength() const
{
	return size_t(length()*2 + 2);
}

#else
/*
	CStr8 is always serialized to/from ASCII (or whatever 8-bit codepage stored
	in the CStr)
*/

u8* CStr8::Serialize(u8* buffer) const
{
	size_t len = length();
	Serialize_int_4(buffer, (u32)len);
	size_t i = 0;
	for (i = 0; i < len; i++)
		buffer[i] = (*this)[i];
	return buffer + len;
}

const u8* CStr8::Deserialize(const u8* buffer, const u8* bufferend)
{
	u32 len;
	Deserialize_int_4(buffer, len);
	if (buffer + len > bufferend)
		return NULL;
	*this = std::string(buffer, buffer + len);
	return buffer + len;
}

size_t CStr::GetSerializedLength() const
{
	return length() + 4;
}

#endif // _UNICODE

// Clean up, to keep the second pass through unidoubler happy
#undef tstringstream
#undef _tstod
#undef _ttoi
#undef _ttol
#undef _istspace
#undef _totlower
#undef _totupper

#endif // CStr_CPP_FIRST
