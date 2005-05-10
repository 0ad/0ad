#include "precompiled.h"

#ifndef CStr_CPP_FIRST
#define CStr_CPP_FIRST

#include "posix.h" // for htons, ntohs
#include "Network/Serialization.h"
#include <cassert>

#define UNIDOUBLER_HEADER "CStr.cpp"
#include "UniDoubler.h"

// Only include these function definitions in the first instance of CStr.cpp:
CStrW::CStrW(const CStr8 &asciStr) : std::wstring(asciStr.begin(), asciStr.end()) {}
CStr8::CStr8(const CStrW &wideStr) : std:: string(wideStr.begin(), wideStr.end()) {}


// UTF conversion code adapted from http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c

static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char trailingBytesForUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5 };
static const u32 offsetsFromUTF8[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL };

CStr8 CStrW::ToUTF8() const
{
	CStr8 result;

	const wchar_t* source = &*begin();
	for (size_t i = 0; i < Length(); ++i)
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
		result += CStr(buf, bytesToWrite);
	}
	return result;
}

static bool isLegalUTF8(const unsigned char *source, int length) {
	unsigned char a;
	const unsigned char *srcptr = source+length;

	switch (length) {
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


CStrW CStr8::FromUTF8() const
{
	CStrW result;

	const unsigned char* source = (const unsigned char*)&*begin();
	const unsigned char* sourceEnd = (const unsigned char*)&*end();
	while (source < sourceEnd)
	{
		wchar_t ch = 0;
		unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
		if (source + extraBytesToRead >= sourceEnd)
		{
			debug_warn("Invalid UTF-8 (fell off end)");
			return L"";
		}

		if (! isLegalUTF8(source, extraBytesToRead+1)) {
			debug_warn("Invalid UTF-8 (illegal data)");
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

#include "CStr.h"
using namespace std;

#include <sstream>

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
	return uint(_ttoi(c_str()));
}

long CStr::ToLong() const
{
	return _ttol(c_str());
}

unsigned long CStr::ToULong() const
{
	return ulong(_ttol(c_str()));
}

float CStr::ToFloat() const
{
	return (float)_tstod(c_str(), NULL);
}

double CStr::ToDouble() const
{
	return _tstod(c_str(), NULL);
}

// Retrieves at most 'len' characters, starting at 'start'
CStr CStr::GetSubstring(size_t start, size_t len) const
{
	return substr(start, len);
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
long CStr::Find(const TCHAR &tchar) const
{
	size_t Pos = find(tchar, 0);

	if (Pos != npos)
		return (long)Pos;

	return -1;
}

// Search the string for another string 
long CStr::Find(const int &start, const TCHAR &tchar) const
{
	size_t Pos = find(tchar, start);

	if (Pos != npos)
		return (long)Pos;

	return -1;
}

long CStr::FindInsensitive(const int &start, const TCHAR &tchar) const { return LCase().Find(start, _totlower(tchar)); }
long CStr::FindInsensitive(const TCHAR &tchar) const { return LCase().Find(_totlower(tchar)); }
long CStr::FindInsensitive(const CStr& Str) const { return LCase().Find(Str.LCase()); }


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
	tstring NewString = *this;
	for (size_t i = 0; i < length(); i++)
		NewString[i] = (TCHAR)_totlower((*this)[i]);

	return NewString;
}

CStr CStr::UpperCase() const
{
	tstring NewString = *this;
	for (size_t i = 0; i < length(); i++)
		NewString[i] = (TCHAR)_totupper((*this)[i]);

	return NewString;
}

// Lazy versions
// code duplication because return by value overhead if they were merely an alias
CStr CStr::LCase() const
{
	tstring NewString = *this;
	for (size_t i = 0; i < length(); i++)
		NewString[i] = (TCHAR)_totlower((*this)[i]);

	return NewString;
}

CStr CStr::UCase() const
{
	tstring NewString = *this;
	for (size_t i = 0; i < length(); i++)
		NewString[i] = (TCHAR)_totupper((*this)[i]);

	return NewString;
}

// Retrieve the substring of the first n characters 
CStr CStr::Left(size_t len) const
{
	assert(len <= length());
	return substr(0, len);
}

// Retrieve the substring of the last n characters
CStr CStr::Right(size_t len) const
{
	assert(len <= length());
	return substr(length()-len, len);
}

// Retrieve the substring following the last occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::AfterLast(const CStr& Str) const
{
	long pos = ReverseFind(Str);
	if (pos == -1)
		return *this;
	else
		return substr(pos + Str.length());
}

// Retrieve the substring preceding the last occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::BeforeLast(const CStr& Str) const
{
	long pos = ReverseFind(Str);
	if (pos == -1)
		return *this;
	else
		return substr(0, pos);
}

// Retrieve the substring following the first occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::AfterFirst(const CStr& Str) const
{
	long pos = Find(Str);
	if (pos == -1)
		return *this;
	else
		return substr(pos + Str.length());
}

// Retrieve the substring preceding the first occurrence of Str
// (or the whole string if it doesn't contain Str)
CStr CStr::BeforeFirst(const CStr& Str) const
{
	long pos = Find(Str);
	if (pos == -1)
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

CStr CStr::UnescapeBackslashes()
{
	// Currently only handle \n and \\, because they're the only interesting ones
	CStr NewString;
	bool escaping = false;
	for (size_t i = 0; i < length(); i++)
	{
		TCHAR ch = (*this)[i];
		if (escaping)
		{
			switch (ch)
			{
			case _T('n'): NewString += _T('\n'); break;
			default: NewString += ch; break;
			}
			escaping = false;
		}
		else
		{
			if (ch == _T('\\'))
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

// Concatenation:

CStr CStr::operator+(const CStr& Str)
{
	return std::operator+(*this, std::tstring(Str));
}

CStr CStr::operator+(const TCHAR* Str)
{
	return std::operator+(*this, std::tstring(Str));
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


CStr::operator const TCHAR*() const
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

u8 *CStrW::Serialize(u8 *buffer) const
{
	size_t len = length();
	size_t i = 0;
	for (i = 0; i < len; i++)
		*(u16 *)(buffer + i*2) = htons((*this)[i]); // convert to network order (big-endian)
	*(u16 *)(buffer+i*2) = 0;
	return buffer+len*2+2;
}

const u8 *CStrW::Deserialize(const u8 *buffer, const u8 *bufferend)
{
	const u16 *strend = (const u16 *)buffer;
	while ((const u8 *)strend < bufferend && *strend) strend++;
	if ((const u8 *)strend >= bufferend) return NULL;

	resize(strend - (const u16 *)buffer);
	size_t i = 0;
	const u16 *ptr = (const u16 *)buffer;

	std::wstring::iterator str = begin();
	while (ptr < strend)
		*(str++) = (TCHAR)ntohs(*(ptr++)); // convert from network order (big-endian)

	return (const u8 *)(strend+1);
}

uint CStr::GetSerializedLength() const
{
	return uint(length()*2 + 2);
}

#else
/*
	CStr8 is always serialized to/from ASCII (or whatever 8-bit codepage stored
	in the CStr)
*/

u8 *CStr8::Serialize(u8 *buffer) const
{
	size_t len = length();
	size_t i = 0;
	for (i = 0; i < len; i++)
		buffer[i] = (*this)[i];
	buffer[i] = 0;
	return buffer+len+1;
}

const u8 *CStr8::Deserialize(const u8 *buffer, const u8 *bufferend)
{
	const u8 *strend = buffer;
	while (strend < bufferend && *strend) strend++;
	if (strend >= bufferend) return NULL;

	*this = std::string(buffer, strend);

	return strend+1;
}

uint CStr::GetSerializedLength() const
{
	return uint(length() + 1);
}

#endif // _UNICODE

#endif // CStr_CPP_FIRST
