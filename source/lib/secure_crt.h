/* Copyright (C) 2015 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * partial implementation of VC8's secure CRT functions
 */

#ifndef INCLUDED_SECURE_CRT
#define INCLUDED_SECURE_CRT

#include <stdarg.h>

#include "lib/status.h"

namespace ERR
{
	const Status STRING_NOT_TERMINATED = -100600;
}

// if the platform lacks a secure CRT implementation, we'll provide one.
#if MSC_VERSION
# define EMULATE_SECURE_CRT 0
#else
# define EMULATE_SECURE_CRT 1
#endif


#if EMULATE_SECURE_CRT

// (conflicts with glibc definitions)
#if !OS_UNIX || OS_MACOSX || OS_OPENBSD
// return length [in characters] of a string, not including the trailing
// null character. to protect against access violations, only the
// first <max_len> characters are examined; if the null character is
// not encountered by then, <max_len> is returned.
// strnlen is available on OpenBSD
#if !OS_OPENBSD
extern size_t strnlen(const char* str, size_t max_len);
#endif
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

extern int vsprintf_s(char* dst, size_t max_dst_chars, const char* fmt, va_list ap) VPRINTF_ARGS(3);
extern int vswprintf_s(wchar_t* dst, size_t max_dst_chars, const wchar_t* fmt, va_list ap) VWPRINTF_ARGS(3);

extern int sprintf_s(char* buf, size_t max_chars, const char* fmt, ...) PRINTF_ARGS(3);
extern int swprintf_s(wchar_t* buf, size_t max_chars, const wchar_t* fmt, ...) WPRINTF_ARGS(3);

// we'd like to avoid deprecation warnings caused by scanf. selective
// 'undeprecation' isn't possible, replacing all stdio declarations with
// our own deprecation scheme is a lot of work, suppressing all deprecation
// warnings would cause important other warnings to be missed, and avoiding
// scanf outright isn't convenient.
// the remaining alternative is using scanf_s where available and otherwise
// defining it to scanf. note that scanf_s has a different API:
// any %s or %c or %[ format specifier's buffer must be followed by a
// size parameter. callers must either avoid these, or provide two codepaths
// (use scanf #if EMULATE_SECURE_CRT, otherwise scanf_s).
#define scanf_s scanf
#define wscanf_s wscanf
#define fscanf_s fscanf
#define fwscanf_s fwscanf
#define sscanf_s sscanf
#define swscanf_s swscanf

#endif	// #if EMULATE_SECURE_CRT
#endif	// #ifndef INCLUDED_SECURE_CRT
