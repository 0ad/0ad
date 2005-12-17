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


[[ This documentation is out of date; CStr is always 8 bits, and CStrW is always
sizeof(wchar_t) (16 or 32). Don't use _T; CStrW constants are just L"string". ]]

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

// Include this section when in unidoubler mode, and when this unicode/ascii
// version has not already been included.
#if defined(IN_UNIDOUBLER) && ( (defined(_UNICODE) && !defined(CSTR_H_U)) || (!defined(_UNICODE) && !defined(CSTR_H_A)) )

#ifdef _UNICODE
#define CSTR_H_U
#else
#define CSTR_H_A
#endif

#include <string>
#include "ps/utf16string.h"

class CStr8;
class CStrW;

// CStr class, the mother of all strings
class CStr: public std::tstring
{
	// The two variations must be friends with each other
#ifdef _UNICODE
	friend class CStr8;
#else
	friend class CStrW;
#endif

public:

	// CONSTRUCTORS

	CStr() {}
	CStr(const CStr& String)	: std::tstring(String) {}
	CStr(const tchar* String)	: std::tstring(String) {}
	CStr(const tchar* String, size_t Length)
								: std::tstring(String, Length) {}
	CStr(const tchar Char)		: std::tstring(1, Char) {} // std::string's constructor is (repeats, chr)
	CStr(std::tstring String)	: std::tstring(String) {}

	// Named constructor, to avoid overload overload.
	static CStr Repeat(CStr String, size_t Reps);
	
	// CStr(8|W) construction from utf16strings, except on MSVC CStrW where
	// CStrW === utf16string
	#if !(MSC_VERSION && defined(_UNICODE))
		CStr(utf16string String) : std::tstring(String.begin(), String.end()) {}
	#endif

	// Transparent CStrW/8 conversion. Non-ASCII characters are not
	// handled correctly.
	#ifndef _UNICODE
		CStr8(const CStrW &wideStr);
	#else
		CStrW(const CStr8 &asciiStr);
	#endif

	// Conversion to/from UTF-8, encoded in a CStr8. Non-ASCII characters are
	// handled correctly.
	// May fail, if converting from invalid UTF-8 data; the empty string will
	// be returned.
	#ifdef _UNICODE
		CStr8 ToUTF8() const;
	#else
		CStrW FromUTF8() const;
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

	// Returns the length of the string in characters
	size_t Length() const { return length(); }

	// Retrieves the substring within the string 
	CStr GetSubstring(size_t start, size_t len) const;

	// Search the string for another string. Returns the offset of the first
	// match, or -1 if no matches are found.
	long Find(const CStr& Str) const;
	long Find(const tchar &chr) const;
	long Find(const int &start, const tchar &chr) const;

	// Case-insensitive versions of Find
	long FindInsensitive(const CStr& Str) const;
	long FindInsensitive(const tchar &chr) const;
	long FindInsensitive(const int &start, const tchar &chr) const;

	// You can also do a "ReverseFind" - i.e. search starting from the end 
	long ReverseFind(const CStr& Str) const;

	// Lowercase and uppercase 
	CStr LowerCase() const;
	CStr UpperCase() const;
	CStr LCase() const;
	CStr UCase() const;

	// Retrieve the substring of the first n characters 
	CStr Left(size_t len) const;

	// Retrieve the substring of the last n characters
	CStr Right(size_t len) const;

	// Retrieve the substring following the last occurrence of Str
	// (or the whole string if it doesn't contain Str)
	CStr AfterLast(const CStr& Str) const;
	
	// Retrieve the substring preceding the last occurrence of Str
	// (or the whole string if it doesn't contain Str)
	CStr BeforeLast(const CStr& Str) const;

	// Retrieve the substring following the first occurrence of Str
	// (or the whole string if it doesn't contain Str)
	CStr AfterFirst(const CStr& Str) const;

	// Retrieve the substring preceding the first occurrence of Str
	// (or the whole string if it doesn't contain Str)
	CStr BeforeFirst(const CStr& Str) const;

	// Remove all occurrences of some character or substring 
	void Remove(const CStr& Str);

	// Replace all occurrences of some substring by another 
	void Replace(const CStr& StrToReplace, const CStr& ReplaceWith);

	// Convert strings like  \\\n  into <backslash><newline>
	CStr UnescapeBackslashes();

	// Returns a trimmed string, removes whitespace from the left/right/both
	CStr Trim(PS_TRIM_MODE Mode) const;

	// Returns a padded string, adding spaces to the left/right/both
	CStr Pad(PS_TRIM_MODE Mode, size_t Length) const;

	// Overloaded operations

	CStr& operator=(int Number);
	CStr& operator=(long Number);
	CStr& operator=(unsigned int Number);
	CStr& operator=(unsigned long Number);
	CStr& operator=(float Number);
	CStr& operator=(double Number);

	CStr  operator+(const CStr& Str);
	CStr  operator+(const tchar* Str);
#ifndef _UNICODE
	CStr8 operator+(const CStrW& Str);
#else
	CStrW operator+(const CStr8& Str);
#endif

	operator const tchar*() const;

	// Do some range checking in debug builds
	tchar& operator[](size_t n)	{ debug_assert(n < length()); return this->std::tstring::operator[](n); }
	tchar& operator[](int n)	{ debug_assert((size_t)n < length()); return this->std::tstring::operator[](n); }
	
	// Conversion to utf16string
	inline utf16string utf16() const
	{	return utf16string(begin(), end()); }

	// Calculates a hash of the string's contents
	size_t GetHashCode() const;

	// Serialization functions
	// (These are not virtual or inherited from ISerializable, to avoid
	// adding a vtable and making the strings larger than std::string)
	uint GetSerializedLength() const;
	u8 *Serialize(u8 *buffer) const;
	const u8 *Deserialize(const u8 *buffer, const u8 *bufferend);
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
