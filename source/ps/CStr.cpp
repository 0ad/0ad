#include "CStr.h"

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

CStr::CStr(_int Number)
{
	// Creates CStr from a int
	m_String = _itot(Number, m_ConversionBuffer, 10);
}

CStr::CStr(_uint Number)
{
	// Creates CStr from a uint
	m_String = _ultot(Number, m_ConversionBuffer, 10);
}


CStr::CStr(_long Number)
{
	// Creates CStr from a long 
	m_String = _ltot(Number, m_ConversionBuffer, 10);
}


CStr::CStr(_ulong Number)
{
	// Creates CStr from a ulong 
	m_String = _ultot(Number, m_ConversionBuffer, 10);
}


CStr::CStr(_float Number)
{
	// Creates CStr from a float
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
}

CStr::CStr(_double Number)
{
	// Creates CStr from a double
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
}

CStr::~CStr()
{
	// Destructor
}


_int CStr::ToInt()
{
	return _ttoi(m_String.c_str());
}

_uint CStr::ToUInt()
{
	return _uint(_ttoi(m_String.c_str()));
}

_long CStr::ToLong()
{
	return _ttol(m_String.c_str());
}

_ulong CStr::ToULong()
{
	return _ulong(_ttol(m_String.c_str()));
}


_float CStr::ToFloat()
{
	return _tstod(m_String.c_str(), NULL);
}

_double	CStr::ToDouble()
{
	return _tstod(m_String.c_str(), NULL);
}

//You can retrieve the substring within the string 
CStr CStr::GetSubstring(_long start, _long len)
{
	return CStr( m_String.substr(start, len) );
}


//Search the string for another string 
_long CStr::Find(const CStr &Str)
{
	_long Pos = m_String.find(Str.m_String, 0);

	if (Pos != tstring::npos)
		return Pos;

	return -1;
}

_long CStr::ReverseFind(const CStr &Str)
{
	_long Pos = m_String.rfind(Str.m_String, m_String.length() );

	if (Pos != tstring::npos)
		return Pos;

	return -1;

}

// Lowercase and uppercase 
CStr CStr::LowerCase()
{
	tstring NewTString = m_String;
	for (long i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

CStr CStr::UpperCase()
{
	tstring NewTString = m_String;
	for (long i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

// Lazy versions
// code duplication because return by value overhead if they were merely an allias
CStr CStr::LCase()
{
	tstring NewTString = m_String;
	for (long i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

CStr CStr::UCase()
{
	tstring NewTString = m_String;
	for (long i = 0; i < m_String.length(); i++)
		NewTString[i] = _totlower(m_String[i]);

	return CStr(NewTString);
}

//Retreive the substring of the first n characters 
CStr CStr::Left(_long len)
{
	return CStr( m_String.substr(0, len) );
}

//Retreive the substring of the last n characters
CStr CStr::Right(_long len)
{
	return CStr( m_String.substr(m_String.length()-len, len) );
}

//Remove all occurences of some character or substring 
void CStr::Remove(const CStr &Str)
{
	long FoundAt = 0;
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
	long Pos = 0;
	
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
	long Left, Right;
	
	
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
			for (Right = m_String.length()-1; Right >= 0; Right--)
				if (_istspace(m_String[Right]) == false)
					break; // end found, trim len-1 to Right+1	inclusive
		} break;
		
		case PS_TRIM_BOTH:
		{
			for (Left = 0; Left < m_String.length(); Left++)
				if (_istspace(m_String[Left]) == false)
					break; // end found, trim 0 to Left-1 inclusive

			for (Right = m_String.length()-1; Right >= 0; Right--)
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

CStr &CStr::operator=(_int Number)
{
	m_String = _itot(Number, m_ConversionBuffer, 10);
	return *this;
}

CStr &CStr::operator=(_long Number)
{
	m_String = _ltot(Number, m_ConversionBuffer, 10);
	return *this;
}

CStr &CStr::operator=(_uint Number)
{
	m_String = _ultot(Number, m_ConversionBuffer, 10);
	return *this;
}

CStr &CStr::operator=(_ulong Number)
{
	m_String = _ultot(Number, m_ConversionBuffer, 10);
	return *this;
}


CStr &CStr::operator=(_float Number)
{
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
	return *this;
}

CStr &CStr::operator=(_double Number)
{
	_tsprintf(m_ConversionBuffer, FLOAT_CONVERSION, Number);
	m_String = m_ConversionBuffer;
	return *this;
}

_bool CStr::operator==(const CStr &Str) const
{
	return (m_String == Str.m_String);
}

_bool CStr::operator!=(const CStr &Str) const
{
	return (m_String != Str.m_String);
}

_bool CStr::operator<(const CStr &Str) const
{
	return (m_String < Str.m_String);
}

_bool CStr::operator<=(const CStr &Str) const
{
	return (m_String <= Str.m_String);
}

_bool CStr::operator>(const CStr &Str) const
{
	return (m_String > Str.m_String);
}

_bool CStr::operator>=(const CStr &Str) const
{
	return (m_String >= Str.m_String);
}

CStr &CStr::operator+=(CStr &Str)
{
	m_String += Str.m_String;
	return *this;
}

CStr CStr::operator+(CStr &Str)
{
	CStr NewStr(*this);
	NewStr.m_String += Str.m_String;
	return NewStr;
}

CStr::operator const TCHAR*()
{
	return m_String.c_str();
}


TCHAR &CStr::operator[](_int n)
{
	assert(n < m_String.length());
	return m_String[n];
}

TCHAR &CStr::operator[](_uint n)
{
	assert(n < m_String.length());
	return m_String[n];
}
TCHAR &CStr::operator[](_long n)
{
	assert(n < m_String.length());
	return m_String[n];
}

TCHAR &CStr::operator[](_ulong n)
{
	assert(n < m_String.length());
	return m_String[n];
}

ostream &operator<<(ostream &os, CStr &Str)
{
	os << (const TCHAR*)Str;
	return os;
}