/**
 * File        : CStr.cpp
 * Project     : engine
 * Description : Controls compilation of CStr class and
 *             : includes some function implementations.
 **/
#include "precompiled.h"

#ifndef CStr_CPP_FIRST
#define CStr_CPP_FIRST

#include "lib/posix/posix_sock.h" // htons, ntohs
#include "lib/fnv_hash.h"
#include "network/Serialization.h"
#include <cassert>

#include <sstream>

#define UNIDOUBLER_HEADER "CStr.cpp"
#include "UniDoubler.h"


// Only include these function definitions in the first instance of CStr.cpp:

CStrW::CStrW(const CStr8 &asciStr) : std::wstring(asciStr.begin(), asciStr.end()) {}
CStr8::CStr8(const CStrW& wideStr) : std:: string(wideStr.begin(), wideStr.end()) {}

// UTF conversion code adapted from http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c

/**
 * Used by ToUTF8
 **/
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
/**
 * Used by FromUTF8
 **/
static const char trailingBytesForUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5 };
/**
 * Used by FromUTF8
 **/
static const u32 offsetsFromUTF8[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/**
 * Convert CStr to UTF-8
 *
 * @return CStr8 converted string
 **/
CStr8 CStrW::ToUTF8() const
{
	CStr8 result;

	for (size_t i = 0; i < length(); ++i)
	{
		unsigned short bytesToWrite;
		wchar_t ch = (*this)[i];

		if (ch < 0x80) bytesToWrite = 1;
		else if (ch < 0x800) bytesToWrite = 2;
		else if (ch < 0x10000) bytesToWrite = 3;
		else if (ch <= 0x7FFFFFFF) bytesToWrite = 4;
		else bytesToWrite = 3, ch = 0x0000FFFD; // replacement character

		char buf[4];
		char* target = &buf[bytesToWrite];
		switch (bytesToWrite)
		{
		case 4: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
		case 3: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
		case 2: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
		case 1: *--target = (ch | firstByteMark[bytesToWrite]);
		}
		result += CStr8(buf, bytesToWrite);
	}

	return result;
}

/**
 * Test for valid UTF-8 string
 *
 * @param const unsigned char * source pointer to string to test.
 * @param int Length of string to test.
 * @return bool true if source string is legal UTF-8,
 *				false if not.
 **/
static bool isLegalUTF8(const unsigned char *source, int Length)
{
	unsigned char a;
	const unsigned char *srcptr = source+Length;

	switch (Length) {
	default: return false;
	case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
	case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
	case 2: if ((a = (*--srcptr)) > 0xBF) return false;

	switch (*source) {
		case 0xE0: if (a < 0xA0) return false; break;
		case 0xED: if (a > 0x9F) return false; break;
		case 0xF0: if (a < 0x90) return false; break;
		case 0xF4: if (a > 0x8F) return false; break;
		default:   if (a < 0x80) return false;
	}
	case 1: if (*source >= 0x80 && *source < 0xC2) return false;
	}

	if (*source > 0xF4) return false;
	return true;
}


/**
 * Convert UTF-8 to CStr
 *
 * @return CStrW converted string
 **/
CStrW CStr8::FromUTF8() const
{
	CStrW result;

	if (empty())
		return result;

	const unsigned char* source = (const unsigned char*)&*begin();
	const unsigned char* sourceEnd = source + length();
	while (source < sourceEnd)
	{
		wchar_t ch = 0;
		unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
		if (source + extraBytesToRead >= sourceEnd)
		{
			//debug_warn("Invalid UTF-8 (fell off end)");
			return L"";
		}

		if (! isLegalUTF8(source, extraBytesToRead+1)) {
			//debug_warn("Invalid UTF-8 (illegal data)");
			return L"";
		}

		switch (extraBytesToRead)
		{
		case 5: ch += *source++; ch <<= 6;
		case 4: ch += *source++; ch <<= 6;
		case 3: ch += *source++; ch <<= 6;
		case 2: ch += *source++; ch <<= 6;
		case 1: ch += *source++; ch <<= 6;
		case 0: ch += *source++;
		}
		ch -= offsetsFromUTF8[extraBytesToRead];

		result += ch;
	}
	return result;
}

#else

// The following code is compiled twice, as CStrW then as CStr8:

#include "CStr.h"

#include <sstream>

#ifdef  _UNICODE
 #define tstringstream wstringstream
 #define _tstod wcstod
 #define _ttoi(a) wcstol(a, NULL, 0)
 #define _ttol(a) wcstol(a, NULL, 0)
 #define _istspace iswspace
 #define _totlower towlower
 #define _totupper towupper
#else
 #define tstringstream stringstream
 #define _tstod strtod
 #define _ttoi atoi
 #define _ttol atol
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

// Construction and assignment from numbers:

#define NUM_TYPE(T) \
	CStr::CStr(T Number)			\
	{								\
		std::tstringstream ss;		\
		ss << Number;				\
		ss >> *this;				\
	}								\
									\
	CStr& CStr::operator=(T Number)	\
	{								\
		std::tstringstream ss;		\
		ss << Number;				\
		ss >> *this;				\
		return *this;				\
	}

NUM_TYPE(int)
NUM_TYPE(long)
NUM_TYPE(unsigned int)
NUM_TYPE(unsigned long)
NUM_TYPE(float)
NUM_TYPE(double)

#undef NUM_TYPE

// Conversion to numbers:

int CStr::ToInt() const
{
	return _ttoi(c_str());
}

unsigned int CStr::ToUInt() const
{
	return (unsigned int)_ttoi(c_str());
}

long CStr::ToLong() const
{
	return _ttol(c_str());
}

unsigned long CStr::ToULong() const
{
	return (unsigned long)_ttol(c_str());
}

float CStr::ToFloat() const
{
	return (float)_tstod(c_str(), NULL);
}

double CStr::ToDouble() const
{
	return _tstod(c_str(), NULL);
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
	debug_assert(len <= length());
	return substr(0, len);
}

// Retrieve the substring of the last n characters
CStr CStr::Right(size_t len) const
{
	debug_assert(len <= length());
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

CStr CStr::UnescapeBackslashes() const
{
	// Currently only handle \n and \\, because they're the only interesting ones
	CStr NewString;
	bool escaping = false;
	for (size_t i = 0; i < length(); i++)
	{
		tchar ch = (*this)[i];
		if (escaping)
		{
			switch (ch)
			{
			case 'n': NewString += '\n'; break;
			default: NewString += ch; break;
			}
			escaping = false;
		}
		else
		{
			if (ch == '\\')
				escaping = true;
			else
				NewString += ch;
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
			debug_warn("CStr::Trim: invalid Mode");
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
		debug_warn("CStr::Trim: invalid Mode");
	}

	return std::tstring(Left, _T(' ')) + *this + std::tstring(Right, _T(' '));
}

// Concatenation:

CStr CStr::operator+(const CStr& Str)
{
	CStr tmp(*this);
	tmp += Str;
	return tmp;
}

CStr CStr::operator+(const tchar* Str)
{
	CStr tmp(*this);
	tmp += Str;
	return tmp;
}

// Joining ASCII and Unicode strings:
#ifndef _UNICODE
CStr8 CStr::operator+(const CStrW& Str)
{
	return std::operator+(*this, CStr8(Str));
}
#else
CStrW CStr::operator+(const CStr8& Str)
{
	return std::operator+(*this, CStrW(Str));
}
#endif


CStr::operator const tchar*() const
{
	return c_str();
}


size_t CStr::GetHashCode() const
{
	return (size_t)fnv_hash(data(), length());
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
		*(u16 *)(buffer + i*2) = htons((*this)[i]); // convert to network order (big-endian)
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
		*(str++) = (tchar)ntohs(*(ptr++)); // convert from network order (big-endian)

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
	size_t i = 0;
	for (i = 0; i < len; i++)
		buffer[i] = (*this)[i];
	buffer[i] = 0;
	return buffer+len+1;
}

const u8* CStr8::Deserialize(const u8* buffer, const u8* bufferend)
{
	const u8 *strend = buffer;
	while (strend < bufferend && *strend) strend++;
	if (strend >= bufferend) return NULL;

	*this = std::string(buffer, strend);

	return strend+1;
}

size_t CStr::GetSerializedLength() const
{
	return size_t(length() + 1);
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
