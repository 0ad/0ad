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
	int MyNum = 126;
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

// history:
// 19 May 04, Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)
// 2004-06-18 janwas: replaced conversion buffer with stringstream

#ifndef CSTR_H_FIRST
#define CSTR_H_FIRST

enum PS_TRIM_MODE {PS_TRIM_LEFT, PS_TRIM_RIGHT, PS_TRIM_BOTH};

#ifndef IN_UNIDOUBLER
#define UNIDOUBLER_HEADER "CStr.h"
#include "UniDoubler.h"
#endif

#endif

#if !defined(CSTR_H) || defined(IN_UNIDOUBLER)
#define CSTR_H

#include "Prometheus.h"
#include <string>				// Used for basic string functionality
#include <iostream>

#include "posix.h"
#include "lib.h"
#include "Network/Serialization.h"

#include <cstdlib>

#ifdef  _UNICODE

#define tstring wstring
#define tstringstream wstringstream
#define _tcout	wcout
#define	_tstod	wcstod
#define TCHAR wchar_t
#define _ttoi _wtoi
#define _ttol _wtol
#define _T(t) L ## t
#define _totlower towlower
#define _istspace iswspace
#define _tsnprintf swprintf

#else

#define tstringstream stringstream
#define tstring string
#define _tcout	cout
#define	_tstod	strtod
#define _ttoi atoi
#define _ttol atol
#define TCHAR char
#define _T(t) t
#define _istspace isspace
#define _tsnprintf snprintf
#define _totlower tolower

#endif

// CStr class, the mother of all strings
class CStr: public ISerializable
{
public:

	// CONSTRUCTORS
	CStr();					// Default constructor
	CStr(const CStr& Str);		// Copy Constructor
	
	CStr(std::tstring String);	// Creates CStr from C++ string
	CStr(const TCHAR* String);	// Creates CStr from C-Style TCHAR string
	CStr(TCHAR Char);		// Creates CStr from a TCHAR
	CStr(int Number);		// Creates CStr from a int
	CStr(unsigned int Number);		// Creates CStr from a uint
	CStr(long Number);		// Creates CStr from a long 
	CStr(unsigned long Number);	// Creates CStr from a ulong 
	CStr(float Number);	// Creates CStr from a float
	CStr(double Number);	// Creates CStr from a double
	~CStr();				// Destructor

	// Conversions
	int	ToInt() const;
	unsigned int	ToUInt() const;
	long	ToLong() const;
	unsigned long	ToULong() const;
	float	ToFloat() const;
	double	ToDouble() const;

	size_t Length() const {return m_String.length();}
	// Retrieves the substring within the string 
	CStr GetSubstring(size_t start, size_t len) const;

	//Search the string for another string 
	long Find(const CStr& Str) const;

	//Search the string for another string 
	long Find(const TCHAR &tchar) const;

	//Search the string for another string 
	long Find(const int &start, const TCHAR &tchar) const;

	//You can also do a "ReverseFind"- i.e. search starting from the end 
	long ReverseFind(const CStr& Str) const;

	// Lowercase and uppercase 
	CStr LowerCase() const;
	CStr UpperCase() const;

	// Lazy funcs
	CStr LCase() const;
	CStr UCase() const;

	// Retreive the substring of the first n characters 
	CStr Left(long len) const;

	// Retreive the substring of the last n characters
	CStr Right(long len) const;
	
	//Remove all occurences of some character or substring 
	void Remove(const CStr& Str);

	//Replace all occurences of some substring by another 
	void Replace(const CStr& StrToReplace, const CStr& ReplaceWith);

	// Returns a trimed string, removes whitespace from the left/right/both
	CStr Trim(PS_TRIM_MODE Mode);

	// Overload operations
	CStr& operator=(const CStr& Str);
	CStr& operator=(const TCHAR* String);
	CStr& operator=(TCHAR Char);
	CStr& operator=(int Number);
	CStr& operator=(long Number);
	CStr& operator=(unsigned int Number);
	CStr& operator=(unsigned long Number);
	CStr& operator=(float Number);
	CStr& operator=(double Number);

	bool operator==(const CStr& Str) const;
	bool operator!=(const CStr& Str) const;
	bool operator<(const CStr& Str) const;
	bool operator<=(const CStr& Str) const;
	bool operator>(const CStr& Str) const;
	bool operator>=(const CStr& Str) const;
	CStr& operator+=(const CStr& Str);
	CStr  operator+(const CStr& Str);
	operator const TCHAR*();
	operator const TCHAR*() const; // Gee, I've added this, Maybe the one above should be removed?
	TCHAR &operator[](int n);
	TCHAR &operator[](unsigned int n);
	TCHAR &operator[](long n);
	TCHAR &operator[](unsigned long n);

	inline const TCHAR *c_str()
	{	return m_String.c_str(); }
	
	size_t GetHashCode() const;

	// Serialization functions
	virtual uint GetSerializedLength() const;
	virtual u8 *Serialize(u8 *buffer) const;
	virtual const u8 *Deserialize(const u8 *buffer, const u8 *bufferend);
	
protected:
	std::tstring m_String;
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
std::ostream &operator<<(std::ostream &os, CStr& Str);

#endif
