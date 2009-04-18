/**
 * =========================================================================
 * File        : secure_crt.cpp
 * Project     : 0 A.D.
 * Description : partial implementation of VC8's secure CRT functions
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_SECURE_CRT
#define INCLUDED_SECURE_CRT

namespace ERR
{
	const LibError STRING_NOT_TERMINATED = -100600;
}

// if the platform lacks a secure CRT implementation, we'll provide one.
#if MSC_VERSION >= 1400
# define EMULATE_SECURE_CRT 0
#else
# define EMULATE_SECURE_CRT 1
#endif


#if EMULATE_SECURE_CRT

// (conflicts with glibc definitions)
#if !OS_UNIX
// return length [in characters] of a string, not including the trailing
// null character. to protect against access violations, only the
// first <max_len> characters are examined; if the null character is
// not encountered by then, <max_len> is returned.
extern size_t strnlen(const char* str, size_t max_len);
extern size_t wcsnlen(const wchar_t* str, size_t max_len);
#endif

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


extern int sprintf_s(char* buf, size_t max_chars, const char* fmt, ...);
extern int swprintf_s(wchar_t* buf, size_t max_chars, const wchar_t* fmt, ...);

typedef int errno_t;
extern errno_t fopen_s(FILE** pfile, const char* filename, const char* mode);
extern errno_t _wfopen_s(FILE** pfile, const wchar_t* filename, const wchar_t* mode);

#define fscanf_s fscanf
#define sscanf_s sscanf

#endif	// #if EMULATE_SECURE_CRT
#endif	// #ifndef INCLUDED_SECURE_CRT
