#include "precompiled.h"

#ifndef CStr_CPP_FIRST
#define CStr_CPP_FIRST

#include "Network/Serialization.h"
#include <cassert>

#define UNIDOUBLER_HEADER "CStr.cpp"
#include "UniDoubler.h"

// Only include these function definitions in the first instance of CStr.cpp:
CStrW::CStrW(const CStr8 &asciStr) : std::wstring(asciStr.begin(), asciStr.end()) {}
CStr8::CStr8(const CStrW &wideStr) : std:: string(wideStr.begin(), wideStr.end()) {}

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
CStr CStr::Left(long len) const
{
	assert(len >= 0);
	assert(len <= (long)length());
	return substr(0, len);
}

// Retrieve the substring of the last n characters
CStr CStr::Right(long len) const
{
	assert(len >= 0);
	assert(len <= (long)length());
	return substr(length()-len, len);
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
	return (size_t)fnv_hash64(data(), length());
		// janwas asks: do we care about the hash being 64 bits?
		// it is truncated here on 32-bit systems; why go 64 bit at all?
}

uint CStr::GetSerializedLength() const
{
	return uint(length()*2 + 2);
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
#endif // _UNICODE

#endif // CStr_CPP_FIRST
