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
// 2004-10-31 Philip: Changed to inherit from std::[w]string

#ifndef CSTR_H_FIRST
#define CSTR_H_FIRST

enum PS_TRIM_MODE { PS_TRIM_LEFT, PS_TRIM_RIGHT, PS_TRIM_BOTH };

#ifndef IN_UNIDOUBLER
 #define UNIDOUBLER_HEADER "CStr.h"
 #include "UniDoubler.h"
#endif

#endif

#if !defined(CSTR_H) || defined(IN_UNIDOUBLER)
#define CSTR_H

#include "Pyrogenesis.h"
#include <string>				// Used for basic string functionality
#include <iostream>
#include "ps/utf16string.h"

#include "Network/Serialization.h"

#include <cstdlib>

#ifdef  _UNICODE

 #define tstring wstring
 #define tstringstream wstringstream
 #define _tcout	wcout
 #define _tstod	wcstod
 #define TCHAR wchar_t
 #define _ttoi(a) wcstol(a, NULL, 0)
 #define _ttol(a) wcstol(a, NULL, 0)
 #define _T(t) L ## t
 #define _istspace iswspace
 #define _tsnprintf swprintf
 #define _totlower towlower
 #define _totupper towupper

#else

 #define tstringstream stringstream
 #define tstring string
 #define _tcout	cout
 #define _tstod	strtod
 #define _ttoi atoi
 #define _ttol atol
 #define TCHAR char
 #define _T(t) t
 #define _istspace isspace
 #define _tsnprintf snprintf
 #define _totlower tolower
 #define _totupper toupper
 
#endif

class CStr8;
class CStrW;

// CStr class, the mother of all strings
class CStr: public std::tstring, public ISerializable
{
#ifdef _UNICODE
	friend class CStr8;
#else
	friend class CStrW;
#endif
public:

	// CONSTRUCTORS

	CStr() {}
	CStr(const CStr& String)	: std::tstring(String) {}
	CStr(const TCHAR* String)	: std::tstring(String) {}
	CStr(const TCHAR* String, size_t Length)
								: std::tstring(String, Length) {}
	CStr(const TCHAR Char)		: std::tstring(1, Char) {} // std::string's constructor is (repeats, chr)
	CStr(std::tstring String)	: std::tstring(String) {}
	
	// CStrW construction from utf16strings
	#if !(defined(_MSC_VER) && defined(_UNICODE))
		CStr(utf16string String) : std::tstring(String.begin(), String.end()) {}
	#endif

	// Transparent CStrW/8 conversion.
	#ifndef _UNICODE
		CStr8(const CStrW &wideStr);
	#else
		CStrW(const CStr8 &asciiStr);
	#endif

	CStr(int Number);			// Creates CStr from a int
	CStr(unsigned int Number);	// Creates CStr from a uint
	CStr(long Number);			// Creates CStr from a long 
	CStr(unsigned long Number);	// Creates CStr from a ulong 
	CStr(float Number);			// Creates CStr from a float
	CStr(double Number);		// Creates CStr from a double

	~CStr() {};					// Destructor

	// Conversions
	int				ToInt() const;
	unsigned int	ToUInt() const;
	long			ToLong() const;
	unsigned long	ToULong() const;
	float			ToFloat() const;
	double			ToDouble() const;

	//  Returns the length of the string in characters
	size_t Length() const { return length(); }

	// Retrieves the substring within the string 
	CStr GetSubstring(size_t start, size_t len) const;

	// Search the string for another string. Returns the offset of the first
	// match, or -1 if no matches are found.
	long Find(const CStr& Str) const;
	long Find(const TCHAR &tchar) const;
	long Find(const int &start, const TCHAR &tchar) const;

	// Case-insensitive versions of Find
	long FindInsensitive(const CStr& Str) const;
	long FindInsensitive(const TCHAR &tchar) const;
	long FindInsensitive(const int &start, const TCHAR &tchar) const;

	// You can also do a "ReverseFind" - i.e. search starting from the end 
	long ReverseFind(const CStr& Str) const;

	// Lowercase and uppercase 
	CStr LowerCase() const;
	CStr UpperCase() const;
	CStr LCase() const;
	CStr UCase() const;

	// Retrieve the substring of the first n characters 
	CStr Left(long len) const;

	// Retrieve the substring of the last n characters
	CStr Right(long len) const;
	
	// Remove all occurrences of some character or substring 
	void Remove(const CStr& Str);

	// Replace all occurrences of some substring by another 
	void Replace(const CStr& StrToReplace, const CStr& ReplaceWith);

	// Convert strings like  \\\n  into <backslash><newline>
	CStr UnescapeBackslashes();

	// Returns a trimmed string, removes whitespace from the left/right/both
	CStr Trim(PS_TRIM_MODE Mode) const;

	// Overloaded operations

	CStr& operator=(int Number);
	CStr& operator=(long Number);
	CStr& operator=(unsigned int Number);
	CStr& operator=(unsigned long Number);
	CStr& operator=(float Number);
	CStr& operator=(double Number);

	CStr  operator+(const CStr& Str);
	CStr  operator+(const TCHAR* Str);
#ifndef _UNICODE
	CStr8 operator+(const CStrW& Str);
#else
	CStrW operator+(const CStr8& Str);
#endif

	operator const TCHAR*() const;

	// Do some range checking in debug builds
	TCHAR &operator[](size_t n)	{ assert(n < length()); return this->std::tstring::operator[](n); }
	TCHAR &operator[](int n)	{ assert(n >= 0 && (size_t)n < length()); return this->std::tstring::operator[](n); }
	
	// Conversion to utf16string
	inline utf16string utf16() const
	{	return utf16string(begin(), end()); }
	
	// Calculates a hash of the string's contents
	size_t GetHashCode() const;

	// Serialization functions
	virtual uint GetSerializedLength() const;
	virtual u8 *Serialize(u8 *buffer) const;
	virtual const u8 *Deserialize(const u8 *buffer, const u8 *bufferend);
};

// Hash function (for STL_HASH_MAP, etc)
class CStr_hash_compare
{
public:
	static const size_t bucket_size = 1;
	static const size_t min_buckets = 16;
	size_t operator() (const CStr& Key) const
	{
		return Key.GetHashCode();
	}
	bool operator() (const CStr& _Key1, const CStr& _Key2) const
	{
		return (_Key1 < _Key2);
	}
};

#endif
