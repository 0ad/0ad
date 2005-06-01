// Make sure we have the argument (UNIDOUBLER_HEADER), and that we're not
// called from within another unidoubler execution (now that's just asking for
// trouble)
#if defined(UNIDOUBLER_HEADER) && !defined(IN_UNIDOUBLER)

#define IN_UNIDOUBLER

// When compiling CStr.cpp with PCH, the unidoubler stuff gets rather
// confusing because of all the nested inclusions, but this makes it work:
#undef CStr
#undef CStr_hash_compare

// First, set up the environment for the Unicode version
#define _UNICODE
#define CStr CStrW
#define CStr_hash_compare CStrW_hash_compare
#define tstring wstring
#define tchar wchar_t
#define _T(t) L ## t

// Include the unidoubled file
#include UNIDOUBLER_HEADER

// Clean up all the macros
#undef _UNICODE
#undef CStr
#undef CStr_hash_compare
#undef tstring
#undef tchar
#undef _T


// Now include the 8-bit version under the name CStr8
#define CStr CStr8
#define CStr_hash_compare CStr8_hash_compare
#define tstring string
#define tchar char
#define _T(t) t

#include UNIDOUBLER_HEADER

// Clean up the macros again, to minimise namespace pollution
#undef CStr
#undef CStr_hash_compare
#undef tstring
#undef tchar
#undef _T


// To please the file that originally include CStr.h, make CStr an alias for CStr8:
#define CStr CStr8
#define CStr_hash_compare CStr8_hash_compare

#undef IN_UNIDOUBLER
#undef UNIDOUBLER_HEADER

#endif
