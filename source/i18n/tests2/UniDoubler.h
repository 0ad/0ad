// Make sure we have the argument (UNIDOUBLER_HEADER), and that we're not
// called from within another unidoubler execution (now that's just asking for
// trouble)
#if defined(UNIDOUBLER_HEADER) && !defined(IN_UNIDOUBLER)

#define IN_UNIDOUBLER

#define _UNICODE
#undef CStr
#undef CStr_hash_compare
#define CStr CStrW
// Compat for old code. *Deprecated* CStrW isn't 16-bit - it is "wide"
#define CStr16 CStrW
#define CStr_hash_compare CStrW_hash_compare
#include UNIDOUBLER_HEADER

// Undef all the Conversion Macros
#undef tstring
#undef tstringstream
#undef _tcout
#undef	_tstod
#undef _ttoi
#undef _ttol
#undef TCHAR
#undef _T
#undef _istspace
#undef _tsnprintf
#undef _totlower
#undef _totupper

// Now include the 8-bit version under the name CStr8
#undef _UNICODE
#undef CStr
#undef CStr_hash_compare
#define CStr CStr8
#define CStr_hash_compare CStr8_hash_compare
#include UNIDOUBLER_HEADER

#undef IN_UNIDOUBLER
#undef UNIDOUBLER_HEADER

#endif

