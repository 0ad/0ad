/*
CStr.h
by Caecus
Caecus@0ad.wildfiregames.com

Overview:

	Contains CStr class which is a versatile class for making string use easy.
	Basic functionality implemented via STL string, so performance is limited
	based on the STL implementation we use.

Example:
	
	CStr MyString = _T("Hello, World.");
	_int MyNum = 126;
	MyString += _T(" I'm the number ") += CStr(MyNumber);
	
	// Prints "Hello, World. I'm the number 126"

    _tcout << (LPCTSTR)MyString << endl;

	MyString = _("2341");
	MyNum = MyString.ToInt();
	
	// Prints "2341"
	_tcout << MyNum << endl;	

More Info:
	http://wildfiregames.com/0ad/codepit/TDD/CStr.html

*/

// Last modified: 19 May 04, Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#ifndef CSTR_H
#define CSTR_H

#include "Prometheus.h"
#include <string>				// Used for basic string functionality
#include <iostream>

#include "posix.h"
#include "lib.h"
#include "Network/Serialization.h"

#include <cstdlib>
using namespace std;

// DEFINES/ENUMS
#define CONVERSION_BUFFER_SIZE		32
#define FLOAT_CONVERSION			_T("%.6f")

#ifdef  _UNICODE
typedef wstring tstring;
#define _tcout	wcout
#define	_tstod	wcstod
typedef wchar_t TCHAR;
#define _ttoi _wtoi
#define _ttol _wtol
#define _itot _itow
#define _ultot _itow
#define _T(t) L ## t

#define _totlower towlower
#define _istspace iswspace
#define _tsprintf wsprintf
#define _ltot _ltow

#else

typedef string	tstring;
#define _tcout	cout
#define	_tstod	strtod
#define _ttoi atoi
#define _ttol atol
#define _itot _itoa
typedef char TCHAR;
#define _T(t) t
#define _istspace isspace
#define _tsprintf sprintf
#define _totlower tolower
#define _ultot _ultoa
#define _ltot _ltoa

#endif

enum PS_TRIM_MODE {PS_TRIM_LEFT, PS_TRIM_RIGHT, PS_TRIM_BOTH};

// yuck - MAX already defines a CStr class...
#define CStr PS_CStr


// CStr class, the mother of all strings
class CStr: public ISerializable
{
public:

	// CONSTRUCTORS
	CStr();					// Default constructor
	CStr(const CStr &Str);		// Copy Constructor
	
	CStr(tstring String);	// Creates CStr from C++ string
	CStr(const TCHAR* String);	// Creates CStr from C-Style TCHAR string
	CStr(TCHAR Char);		// Creates CStr from a TCHAR
	CStr(_int Number);		// Creates CStr from a int
	CStr(_uint Number);		// Creates CStr from a uint
	CStr(_long Number);		// Creates CStr from a long 
	CStr(_ulong Number);	// Creates CStr from a ulong 
	CStr(_float Number);	// Creates CStr from a float
	CStr(_double Number);	// Creates CStr from a double
	~CStr();				// Destructor

	// Conversions
	_int	ToInt() const;
	_uint	ToUInt() const;
	_long	ToLong() const;
	_ulong	ToULong() const;
	_float	ToFloat() const;
	_double	ToDouble() const;

	size_t Length() const {return m_String.length();}
	// Retrieves the substring within the string 
	CStr GetSubstring(size_t start, size_t len) const;

	//Search the string for another string 
	_long Find(const CStr &Str) const;

	//Search the string for another string 
	_long Find(const TCHAR &tchar) const;

	//Search the string for another string 
	_long Find(const int &start, const TCHAR &tchar) const;

	//You can also do a "ReverseFind"- i.e. search starting from the end 
	_long ReverseFind(const CStr &Str) const;

	// Lowercase and uppercase 
	CStr LowerCase() const;
	CStr UpperCase() const;

	// Lazy funcs
	CStr LCase() const;
	CStr UCase() const;

	// Retreive the substring of the first n characters 
	CStr Left(_long len) const;

	// Retreive the substring of the last n characters
	CStr Right(_long len) const;
	
	//Remove all occurences of some character or substring 
	void Remove(const CStr &Str);

	//Replace all occurences of some substring by another 
	void Replace(const CStr &StrToReplace, const CStr &ReplaceWith);

	// Returns a trimed string, removes whitespace from the left/right/both
	CStr Trim(PS_TRIM_MODE Mode);

	// Overload operations
	CStr &operator=(const CStr &Str);
	CStr &operator=(const TCHAR* String);
	CStr &operator=(TCHAR Char);
	CStr &operator=(_int Number);
	CStr &operator=(_long Number);
	CStr &operator=(_uint Number);
	CStr &operator=(_ulong Number);
	CStr &operator=(_float Number);
	CStr &operator=(_double Number);

	_bool operator==(const CStr &Str) const;
	_bool operator!=(const CStr &Str) const;
	_bool operator<(const CStr &Str) const;
	_bool operator<=(const CStr &Str) const;
	_bool operator>(const CStr &Str) const;
	_bool operator>=(const CStr &Str) const;
	CStr &operator+=(const CStr &Str);
	CStr  operator+(const CStr &Str);
	operator const TCHAR*();
	operator const TCHAR*() const; // Gee, I've added this, Maybe the one above should be removed?
	TCHAR &operator[](_int n);
	TCHAR &operator[](_uint n);
	TCHAR &operator[](_long n);
	TCHAR &operator[](_ulong n);

	inline const char *c_str()
	{	return m_String.c_str(); }
	
	size_t GetHashCode() const;

	// Serialization functions
	virtual uint GetSerializedLength() const;
	virtual u8 *Serialize(u8 *buffer) const;
	virtual const u8 *Deserialize(const u8 *buffer, const u8 *bufferend);
	
private:
	tstring m_String;
	TCHAR m_ConversionBuffer[CONVERSION_BUFFER_SIZE];
};

class CStr_hash_compare
{
public:
	static const size_t bucket_size = 1;
	static const size_t min_buckets = 16;
	size_t operator()( const CStr& Key ) const
	{
		return( Key.GetHashCode() );
	}
	bool operator()( const CStr& _Key1, const CStr& _Key2 ) const
	{
		return( _Key1 < _Key2 );
	}
};

// overloaded operator for ostreams
ostream &operator<<(ostream &os, CStr &Str);

#endif
