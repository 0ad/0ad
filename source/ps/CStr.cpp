#include "precompiled.h"

#include "CStr.h"
#include "Network/Serialization.h"
#include <cassert>

CStr::CStr()
{
	// Default Constructor
}

CStr::CStr(const CStr &Str)
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

CStr::CStr(TCHAR Char)
{
	// Creates CStr from a TCHAR
	m_String = Char;
}

CStr::CStr(int Number)
{
	// Creates CStr from a int
	m_String = _itot(Number, m_ConversionBuffer, 10);
}

CStr::CStr(unsigned int Number)
{
	// Creates CStr from a uint
	m_String = _ultot(Number, m_ConversionBuffer, 10);
}


CStr::CStr(long Number)
{
	// Creates CStr from a long 
	m_String = _ltot(Number, m_ConversionBuffer, 10);
}


CStr::CStr(unsigned long Number)
{
	// Creates CStr from a ulong 
	m_String = _ultot(Number, m_ConversionBuffer, 10);
}


CStr::CStr(float Number)
{
	// Creates CStr from a float
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
}

CStr::CStr(double Number)
{
	// Creates CStr from a double
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
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
long CStr::Find(const CStr &Str) const
{
	long Pos = (long)m_String.find(Str.m_String, 0);

	if (Pos != tstring::npos)
		return Pos;

	return -1;
}

//Search the string for another string 
long CStr::Find(const TCHAR &tchar) const
{
	long Pos = (long)m_String.find(tchar, 0);

	if (Pos != tstring::npos)
		return Pos;

	return -1;
}

//Search the string for another string 
long CStr::Find(const int &start, const TCHAR &tchar) const
{
	long Pos = (long)m_String.find(tchar, start);

	if (Pos != tstring::npos)
		return Pos;

	return -1;
}

long CStr::ReverseFind(const CStr &Str) const
{
	long Pos = (long)m_String.rfind(Str.m_String, m_String.length() );

	if (Pos != tstring::npos)
		return Pos;

	return -1;

}

// Lowercase and uppercase 
CStr CStr::LowerCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

CStr CStr::UpperCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

// Lazy versions
// code duplication because return by value overhead if they were merely an allias
CStr CStr::LCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

CStr CStr::UCase() const
{
	tstring NewTString = m_String;
	for (size_t i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

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
void CStr::Remove(const CStr &Str)
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
void CStr::Replace(const CStr &ToReplace, const CStr &ReplaceWith)
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
	size_t Left, Right;
	
	
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
	}


	return CStr( m_String.substr(Left, Right-Left+1) );
}

// Overload operations
CStr &CStr::operator=(const CStr &Str)
{
	m_String = Str.m_String;
	return *this;
}

CStr &CStr::operator=(const TCHAR* String)
{
	m_String = String;
	return *this;
}

CStr &CStr::operator=(TCHAR Char)
{
	m_String = Char;
	return *this;
}

CStr &CStr::operator=(int Number)
{
	m_String = _itot(Number, m_ConversionBuffer, 10);
	return *this;
}

CStr &CStr::operator=(long Number)
{
	m_String = _ltot(Number, m_ConversionBuffer, 10);
	return *this;
}

CStr &CStr::operator=(unsigned int Number)
{
	m_String = _ultot(Number, m_ConversionBuffer, 10);
	return *this;
}

CStr &CStr::operator=(unsigned long Number)
{
	m_String = _ultot(Number, m_ConversionBuffer, 10);
	return *this;
}


CStr &CStr::operator=(float Number)
{
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
	return *this;
}

CStr &CStr::operator=(double Number)
{
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
	return *this;
}

bool CStr::operator==(const CStr &Str) const
{
	return (m_String == Str.m_String);
}

bool CStr::operator!=(const CStr &Str) const
{
	return (m_String != Str.m_String);
}

bool CStr::operator<(const CStr &Str) const
{
	return (m_String < Str.m_String);
}

bool CStr::operator<=(const CStr &Str) const
{
	return (m_String <= Str.m_String);
}

bool CStr::operator>(const CStr &Str) const
{
	return (m_String > Str.m_String);
}

bool CStr::operator>=(const CStr &Str) const
{
	return (m_String >= Str.m_String);
}

CStr &CStr::operator+=(const CStr &Str)
{
	m_String += Str.m_String;
	return *this;
}

CStr CStr::operator+(const CStr &Str)
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

ostream &operator<<(ostream &os, CStr &Str)
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
	return uint(m_String.length()+1);
}

u8 *CStr::Serialize(u8 *buffer) const
{
	size_t length=m_String.length();
	memcpy(buffer, m_String.c_str(), length+1);
	return buffer+length+1;
}

const u8 *CStr::Deserialize(const u8 *buffer, const u8 *bufferend)
{
	u8 *strend=(u8 *)memchr(buffer, 0, bufferend-buffer);
	if (strend == NULL)
		return NULL;
	*this=(char *)buffer;
	return strend+1;
}
