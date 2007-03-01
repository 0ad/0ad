/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _PLATFORMS_H_
#define _PLATFORMS_H_

#ifdef FCOLLADA_DLL
// Disable the "private member not available for export" warning,
// because I don't feel like writing interfaces
#pragma warning(disable:4251) 
#ifdef FCOLLADA_INTERNAL
#define FCOLLADA_EXPORT __declspec(dllexport)
#else // FCOLLADA_INTERNAL
#define FCOLLADA_EXPORT __declspec(dllimport)
#endif // FCOLLADA_INTERNAL
#else // FCOLLADA_DLL
#define FCOLLADA_EXPORT
#endif // FCOLLADA_DLL

#ifdef __PPU__
#define UNICODE
#endif // __PPU__

// Ensure that both UNICODE and _UNICODE are set.
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#define UNICODE
#endif
#endif

#include <math.h>

#ifdef WIN32
#pragma warning(disable:4702)
#include <windows.h>
#else
#ifdef MAC_TIGER
#include <ctype.h>
#include <wctype.h>
#else // MAC_TIGER
#if defined(LINUX) || defined(__PPU__)
#else // LINUX
#error "Unsupported platform."
#endif // LINUX || __PPU__
#endif // MAC_TIGER

#endif // WIN32

// Cross-platform type definitions
#ifdef WIN32

typedef signed char int8;
typedef short int16;
typedef long int32;
typedef __int64 int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned __int64 uint64;

#else // For LINUX and MAC_TIGER

typedef signed char int8;
typedef short int16;
typedef long int32;
typedef long long int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

#endif

// Important functions that some OSes have missing!
#if defined(MAC_TIGER) || defined (LINUX)
inline char* strlower(char* str) { char* it = str; while (*it != 0) { *it = tolower(*it); ++it; } return str; }
inline wchar_t* wcslwr(wchar_t* str) { wchar_t* it = str; while (*it != 0) { *it = towlower(*it); ++it; } return str; }
inline int wcsicmp(const wchar_t* s1, const wchar_t* s2) { wchar_t c1 = *s1, c2 = *s2; while (c1 != 0 && c2 != 0) { if (c1 >= 'a' && c1 <= 'z') c1 -= 'a' + 'A'; if (c2 >= 'a' && c2 <= 'z') c2 -= 'a' + 'A'; if (c2 < c1) return -1; else if (c2 > c1) return 1; c1 = *(++s1); c2 = *(++s2); } return 0; }
#ifndef isinf
#define isinf __isinff
#endif
#define _stricmp strcasecmp
#define _getcwd getcwd
#define _chdir chdir

#elif defined(__PPU__)
#define glClearDepth glClearDepthf

#endif // MAC_TIGER and LINUX

// Cross-platform needed functions
#ifdef WIN32

#define vsnprintf _vsnprintf
#define snprintf _snprintf
#define vsnwprintf _vsnwprintf
#define snwprintf _snwprintf
#define strlower _strlwr

#else // WIN32

#define vsnwprintf vswprintf
#define snwprintf swprintf

#endif // WIN32

// For Doxygen purposes, we stopped using the "using namespace std;" statement and use shortcuts instead.

// fstring and character definition
#ifdef UNICODE
#ifdef WIN32

#include <tchar.h>
	typedef TCHAR fchar;
	typedef std::basic_string<fchar> fstring;
	#define FC(a) __T(a)

	#define fstrlen _tcslen
	#define fstrcmp _tcscmp
	#define fstricmp _tcsicmp
	#define fstrncpy _tcsncpy
	#define fstrrchr _tcsrchr
	#define fstrlower _tcslwr
	#define fsnprintf _sntprintf
	#define fvsnprintf _vsntprintf

	#define fchdir _tchdir

#elif __PPU__

	#define fchar wchar_t
	typedef std::wstring fstring;
	#define FC(a) L ## a

	#define fstrlen wcslen
	#define fstrcmp wcscmp
	#define fstricmp wcscmp		// [claforte] TODO: Implement __PPU__ version of wcsicmp
	#define fstrncpy wcsncpy
	#define fstrrchr wcsrchr
	#define fstrlower(x) iswlower(*(x))
	#define fsnprintf swprintf
	#define fvsnprintf vswprintf

	#define fchdir(a) chdir(FUStringConversion::ToString(a).c_str())

#else // For MacOSX and Linux platforms

	#define fchar wchar_t
	typedef std::wstring fstring;
	#define FC(a) L ## a

	#define fstrlen wcslen
	#define fstrcmp wcscmp
	#define fstricmp wcsicmp
	#define fstrncpy wcsncpy
	#define fstrrchr wcsrchr
	#define fstrlower wcslwr
	#define fsnprintf swprintf
	#define fvsnprintf vswprintf

	#define fchdir(a) chdir(FUStringConversion::ToString(a).c_str())

#endif // WIN32

#else // UNICODE

typedef char fchar;
typedef std::basic_string<fchar> fstring;
#define FC(a) a

#define fstrlen strlen
#define fstrcmp strcmp
#define fstricmp _stricmp
#define fstrncpy strncpy
#define fstrrchr strrchr
#define fstrlower strlower
#define fsnprintf snprintf
#define fvsnprintf vsnprintf

#define fatol atol
#define fatof atof
#define fchdir chdir

#endif // UNICODE

#ifndef WIN32
#define MAX_PATH 1024
#endif // !WIN32

#endif // _PLATFORMS_H_
