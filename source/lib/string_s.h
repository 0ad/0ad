#ifndef STRINGS_S_H__
#define STRINGS_S_H__

#include "posix.h"	// size_t

// these are already shipped with VC2005
#if _MSC_VER < 1400


// return length [in characters] of a string, not including the trailing
// null character. to protect against access violations, only the
// first <max_len> characters are examined; if the null character is
// not encountered by then, <max_len> is returned.
extern size_t strlen_s(const char* str, size_t max_len);
extern size_t wcslen_s(const wchar_t* str, size_t max_len);

// copy at most <max_src_chars> (not including trailing null) from
// <src> into <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
//
// note: padding with zeroes is not called for by NG1031.
extern int strncpy_s(char* dst, size_t max_dst_chars, const char* src, size_t max_src_chars);
extern int wcsncpy_s(wchar_t* dst, size_t max_dst_chars, const wchar_t* src, size_t max_src_chars);

// copy <src> (including trailing null) into <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
//
// note: implemented as tncpy_s(dst, max_dst_chars, src, SIZE_MAX)
extern int strcpy_s(char* dst, size_t max_dst_chars, const char* src);
extern int wcscpy_s(wchar_t* dst, size_t max_dst_chars, const wchar_t* src);

// append at most <max_src_chars> (not including trailing null) from
// <src> to <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
extern int strncat_s(char* dst, size_t max_dst_chars, const char* src, size_t max_src_chars);
extern int wcsncat_s(wchar_t* dst, size_t max_dst_chars, const wchar_t* src, size_t max_src_chars);

// append <src> to <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
//
// note: implemented as tncat_s(dst, max_dst_chars, src, SIZE_MAX)
extern int strcat_s(char* dst, size_t max_dst_chars, const char* src);
extern int wcscat_s(wchar_t* dst, size_t max_dst_chars, const wchar_t* src);


#endif	// #if _MSC_VER < 1400

#endif	// #ifndef STRINGS_S_H__
