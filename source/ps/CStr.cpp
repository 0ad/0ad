#include "precompiled.h"

#ifndef CStr_CPP_FIRST
#define CStr_CPP_FIRST

#include "Network/Serialization.h"
#include <cassert>

#define UNIDOUBLER_HEADER "CStr.cpp"
#include "UniDoubler.h"

#else

#include "CStr.h"
using namespace std;

#include <sstream>

CStr::CStr()
{
	// Default Constructor
}

CStr::CStr(const CStr& Str)
{
	// Copy Constructor
	m_String = Str.m_String;
}

CStr::CStr(const TCHAR* String)
{
	// Creates CStr from C-Style TCHAR string
	m_String = String;
}

CStr::CStr(tstring String)
{
	m_String = String;
}

#if !(defined(_MSC_VER) && defined(_UNICODE))
CStr::CStr(std::utf16string String)
{
	m_String = tstring(String.begin(), String.end());
}
#endif

CStr::CStr(TCHAR Char)
{
	// Creates CStr from a TCHAR
	m_String = Char;
}

CStr::CStr(int Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
}

CStr::CStr(unsigned int Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
}


CStr::CStr(long Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
}


CStr::CStr(unsigned long Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
}


CStr::CStr(float Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
}

CStr::CStr(double Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
}

CStr::~CStr()
{
	// Destructor
}


int CStr::ToInt() const
{
	return _ttoi(m_String.c_str());
}

unsigned int CStr::ToUInt() const
{
	return uint(_ttoi(m_String.c_str()));
}

long CStr::ToLong() const
{
	return _ttol(m_String.c_str());
}

unsigned long CStr::ToULong() const
{
	return ulong(_ttol(m_String.c_str()));
}


float CStr::ToFloat() const
{
	return (float)_tstod(m_String.c_str(), NULL);
}

double	CStr::ToDouble() const
{
	return _tstod(m_String.c_str(), NULL);
}

//You can retrieve the substring within the string 
CStr CStr::GetSubstring(size_t start, size_t len) const
{
	return CStr( m_String.substr(start, len) );
}


//Search the string for another string 
long CStr::Find(const CStr& Str) const
{
	size_t Pos = m_String.find(Str.m_String, 0);

	if (Pos != tstring::npos)
		return (long)Pos;

	return -1;
}

//Search the string for another string 
long CStr::Find(const TCHAR &tchar) const
{
	size_t Pos = m_String.find(tchar, 0);

	if (Pos != tstring::npos)
		return (long)Pos;

	return -1;
}

//Search the string for another string 
long CStr::Find(const int &start, const TCHAR &tchar) const
{
	size_t Pos = m_String.find(tchar, start);

	if (Pos != tstring::npos)
		return (long)Pos;

	return -1;
}

long CStr::ReverseFind(const CStr& Str) const
{
	size_t Pos = m_String.rfind(Str.m_String, m_String.length() );

	if (Pos != tstring::npos)
		return (long)Pos;

	return -1;

}

// Lowercase and uppercase 
CStr CStr::LowerCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = (TCHAR)_totlower(m_String[i]);

	return CStr(NewTString);
}

CStr CStr::UpperCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = (TCHAR)_totlower(m_String[i]);

	return CStr(NewTString);
}

// Lazy versions
// code duplication because return by value overhead if they were merely an allias
CStr CStr::LCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = (TCHAR)_totlower(m_String[i]);

	return CStr(NewTString);
}

CStr CStr::UCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = (TCHAR)_totlower(m_String[i]);

	return CStr(NewTString);
}

//Retreive the substring of the first n characters 
CStr CStr::Left(long len) const
{
	return CStr( m_String.substr(0, len) );
}

//Retreive the substring of the last n characters
CStr CStr::Right(long len) const
{
	return CStr( m_String.substr(m_String.length()-len, len) );
}

//Remove all occurences of some character or substring 
void CStr::Remove(const CStr& Str)
{
	size_t FoundAt = 0;
	while (FoundAt != tstring::npos)
	{
		FoundAt = m_String.find(Str.m_String, 0);
		
		if (FoundAt != tstring::npos)
			m_String.erase(FoundAt, Str.m_String.length());
	}
}

//Replace all occurences of some substring by another 
void CStr::Replace(const CStr& ToReplace, const CStr& ReplaceWith)
{
	size_t Pos = 0;
	
	while (Pos != tstring::npos)
	{
		Pos = m_String.find(ToReplace.m_String, Pos);
		if (Pos != tstring::npos)
		{
			m_String.erase(Pos, ToReplace.m_String.length());
			m_String.insert(Pos, ReplaceWith.m_String);
			Pos += ReplaceWith.m_String.length();
		}
	}
}

// returns a trimed string, removes whitespace from the left/right/both
CStr CStr::Trim(PS_TRIM_MODE Mode)
{
	size_t Left = 0, Right = 0;
	
	
	switch (Mode)
	{
		case PS_TRIM_LEFT:
		{
			for (Left = 0; Left < m_String.length(); Left++)
				if (_istspace(m_String[Left]) == false)
					break; // end found, trim 0 to Left-1 inclusive
		} break;
		
		case PS_TRIM_RIGHT:
		{
			Right = m_String.length();
			while (Right--)
				if (_istspace(m_String[Right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;
		
		case PS_TRIM_BOTH:
		{
			for (Left = 0; Left < m_String.length(); Left++)
				if (_istspace(m_String[Left]) == false)
					break; // end found, trim 0 to Left-1 inclusive

			Right = m_String.length();
			while (Right--)
				if (_istspace(m_String[Right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;

		default:
			debug_warn("CStr::Trim: invalid Mode");
	}


	return CStr( m_String.substr(Left, Right-Left+1) );
}

// Overload operations
CStr& CStr::operator=(const CStr& Str)
{
	m_String = Str.m_String;
	return *this;
}

CStr& CStr::operator=(const TCHAR* String)
{
	m_String = String;
	return *this;
}

CStr& CStr::operator=(TCHAR Char)
{
	m_String = Char;
	return *this;
}

CStr& CStr::operator=(int Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;

	return *this;
}

CStr& CStr::operator=(long Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;

	return *this;
}

CStr& CStr::operator=(unsigned int Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;

	return *this;
}

CStr& CStr::operator=(unsigned long Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;

	return *this;
}


CStr& CStr::operator=(float Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
	return *this;
}

CStr& CStr::operator=(double Number)
{
	std::tstringstream ss;
	ss << Number;
	ss >> m_String;
	return *this;
}

bool CStr::operator==(const CStr& Str) const
{
	return (m_String == Str.m_String);
}

bool CStr::operator!=(const CStr& Str) const
{
	return (m_String != Str.m_String);
}

bool CStr::operator<(const CStr& Str) const
{
	return (m_String < Str.m_String);
}

bool CStr::operator<=(const CStr& Str) const
{
	return (m_String <= Str.m_String);
}

bool CStr::operator>(const CStr& Str) const
{
	return (m_String > Str.m_String);
}

bool CStr::operator>=(const CStr& Str) const
{
	return (m_String >= Str.m_String);
}

CStr& CStr::operator+=(const CStr& Str)
{
	m_String += Str.m_String;
	return *this;
}

CStr CStr::operator+(const CStr& Str)
{
	CStr NewStr(*this);
	NewStr.m_String += Str.m_String;
	return NewStr;
}

CStr::operator const TCHAR*()
{
	return m_String.c_str();
}

CStr::operator const TCHAR*() const
{
	return m_String.c_str();
}


TCHAR &CStr::operator[](int n)
{
	assert((size_t)n < m_String.length());
	return m_String[n];
}

TCHAR &CStr::operator[](unsigned int n)
{
	assert(n < m_String.length());
	return m_String[n];
}
TCHAR &CStr::operator[](long n)
{
	assert((size_t)n < m_String.length());
	return m_String[n];
}

TCHAR &CStr::operator[](unsigned long n)
{
	assert(n < m_String.length());
	return m_String[n];
}

ostream &operator<<(ostream &os, CStr& Str)
{
	os << (const TCHAR*)Str;
	return os;
}

size_t CStr::GetHashCode() const
{
	return (size_t)fnv_hash64(m_String.data(), m_String.length());
		// janwas asks: do we care about the hash being 64 bits?
		// it is truncated here on 32-bit systems; why go 64 bit at all?
}

uint CStr::GetSerializedLength() const
{
	return uint(m_String.length()*2 + 2);
}

u8 *CStr::Serialize(u8 *buffer) const
{
	size_t length=m_String.length();
	size_t i=0;
	for (i=0;i<length;i++)
		*(u16 *)(buffer+i*2)=htons(m_String[i]);
	*(u16 *)(buffer+i*2)=0;
	return buffer+length*2+2;
}

const u8 *CStr::Deserialize(const u8 *buffer, const u8 *bufferend)
{
	const u16 *strend=(const u16 *)buffer;
	while ((const u8 *)strend < bufferend && *strend) strend++;
	if ((const u8 *)strend >= bufferend) return NULL;

	m_String.resize(strend-((const u16 *)buffer));
	size_t i=0;
	const u16 *ptr=(const u16 *)buffer;
	while (ptr<strend)
		m_String[i++]=(TCHAR)ntohs(*(ptr++));

	return (const u8 *)(strend+1);
}

#endif
