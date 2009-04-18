/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * partial implementation of VC8's secure CRT functions
 */

#include "precompiled.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "secure_crt.h"


// we were included from wsecure_crt.cpp; skip all stuff that
// must only be done once.
#ifndef WSECURE_CRT
ERROR_ASSOCIATE(ERR::STRING_NOT_TERMINATED, "Invalid string (no 0 terminator found in buffer)", -1);
#endif


// written against http://std.dkuug.dk/jtc1/sc22/wg14/www/docs/n1031.pdf .
// optimized for size - e.g. strcpy calls strncpy with n = SIZE_MAX.

// since char and wide versions of these functions are basically the same,
// this source file implements generic versions and bridges the differences
// with these macros. wsecure_crt.cpp #defines WSECURE_CRT and
// includes this file.
#ifdef WSECURE_CRT
# define tchar wchar_t
# define T(string_literal) L ## string_literal
# define tnlen wcsnlen
# define tncpy_s wcsncpy_s
# define tcpy_s wcscpy_s
# define tncat_s wcsncat_s
# define tcat_s wcscat_s
# define tcmp wcscmp
# define tcpy wcscpy
# define tprintf_s swprintf_s
# define vtnprintf vswprintf    // private
#else
# define tchar char
# define T(string_literal) string_literal
# define tnlen strnlen
# define tncpy_s strncpy_s
# define tcpy_s strcpy_s
# define tncat_s strncat_s
# define tcat_s strcat_s
# define tcmp strcmp
# define tcpy strcpy
# define tprintf_s sprintf_s
# define vtnprintf vsnprintf    // private
#endif	// #ifdef WSECURE_CRT


// return <retval> and raise an assertion if <condition> doesn't hold.
// usable as a statement.
#define ENFORCE(condition, err_to_warn,	retval) STMT(\
	if(!(condition))                    \
	{                                   \
		DEBUG_WARN_ERR(err_to_warn);    \
		return retval;                  \
	}                                   \
)

// raise a debug warning if <len> is the size of a pointer.
// catches bugs such as: tchar* s = ..; tcpy_s(s, sizeof(s), T(".."));
// if warnings get annoying, replace with debug_printf. usable as a statement.
//
// currently disabled due to high risk of false positives.
#define WARN_IF_PTR_LEN(len)\
/*
	debug_assert(len != sizeof(char*));
*/


// skip our implementation if already available, but not the
// self-test and the t* defines (needed for test).
#if EMULATE_SECURE_CRT

#if !OS_UNIX || OS_MACOSX
// return length [in characters] of a string, not including the trailing
// null character. to protect against access violations, only the
// first <max_len> characters are examined; if the null character is
// not encountered by then, <max_len> is returned.
size_t tnlen(const tchar* str, size_t max_len)
{
	// note: we can't bail - what would the return value be?
	debug_assert(str != 0);

	WARN_IF_PTR_LEN(max_len);

	size_t len;
	for(len = 0; len < max_len; len++)
		if(*str++ == '\0')
			break;

	return len;
}
#endif // !OS_UNIX


// copy at most <max_src_chars> (not including trailing null) from
// <src> into <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
//
// note: padding with zeroes is not called for by NG1031.
int tncpy_s(tchar* dst, size_t max_dst_chars, const tchar* src, size_t max_src_chars)
{
	// the MS implementation returns EINVAL and allows dst = 0 if
	// max_dst_chars = max_src_chars = 0. no mention of this in
	// 3.6.2.1.1, so don't emulate that behavior.
	ENFORCE(dst != 0, ERR::INVALID_PARAM, EINVAL);
	ENFORCE(max_dst_chars != 0, ERR::INVALID_PARAM, ERANGE);
	*dst = '\0';	// in case src ENFORCE is triggered
	ENFORCE(src != 0, ERR::INVALID_PARAM, EINVAL);

	WARN_IF_PTR_LEN(max_dst_chars);
	WARN_IF_PTR_LEN(max_src_chars);

	// copy string until null character encountered or limit reached.
	// optimized for size (less comparisons than MS impl) and
	// speed (due to well-predicted jumps; we don't bother unrolling).
	tchar* p = dst;
	size_t chars_left = std::min(max_dst_chars, max_src_chars);
	while(chars_left != 0)
	{
		// success: reached end of string normally.
		if((*p++ = *src++) == '\0')
			return 0;
		chars_left--;
	}

	// which limit did we hit?
	// .. dst, and last character wasn't null: overflow.
	if(max_dst_chars <= max_src_chars)
	{
		*dst = '\0';
		ENFORCE(0, ERR::BUF_SIZE, ERANGE);
	}
	// .. source: success, but still need to null-terminate the destination.
	*p = '\0';
	return 0;
}


// copy <src> (including trailing null) into <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
int tcpy_s(tchar* dst, size_t max_dst_chars, const tchar* src)
{
	return tncpy_s(dst, max_dst_chars, src, SIZE_MAX);
}


// append <src> to <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
int tncat_s(tchar* dst, size_t max_dst_chars, const tchar* src, size_t max_src_chars)
{
	ENFORCE(dst != 0, ERR::INVALID_PARAM, EINVAL);
	ENFORCE(max_dst_chars != 0, ERR::INVALID_PARAM, ERANGE);
	// src is checked in tncpy_s

	// WARN_IF_PTR_LEN not necessary: both max_dst_chars and max_src_chars
	// are checked by tnlen / tncpy_s (respectively).

	const size_t dst_len = tnlen(dst, max_dst_chars);
	if(dst_len == max_dst_chars)
	{
		*dst = '\0';
		ENFORCE(0, ERR::STRING_NOT_TERMINATED, ERANGE);
	}

	tchar* const end = dst+dst_len;
	const size_t chars_left = max_dst_chars-dst_len;
	int ret = tncpy_s(end, chars_left, src, max_src_chars);
	// if tncpy_s overflowed, we need to clear the start of our string
	// (not just the appended part). can't do that by default, because
	// the beginning of dst is not changed in normal operation.
	if(ret != 0)
		*dst = '\0';
	return ret;
}


// append <src> to <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
//
// note: implemented as tncat_s(dst, max_dst_chars, src, SIZE_MAX)
int tcat_s(tchar* dst, size_t max_dst_chars, const tchar* src)
{
	return tncat_s(dst, max_dst_chars, src, SIZE_MAX);
}


int tprintf_s(tchar* buf, size_t max_chars, const tchar* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vtnprintf(buf, max_chars, fmt, args);
	va_end(args);
	return len;
}

// note: there is no portable wfopen, so we need separate implementations
// of tfopen_s. (the Unicode version just converts to UTF8)
#if defined(WSECURE_CRT)

errno_t _wfopen_s(FILE** pfile, const wchar_t* filename, const wchar_t* mode)
{
	*pfile = NULL;

	size_t numConverted;
	char filename_c[PATH_MAX];
	numConverted = wcstombs(filename_c, filename, PATH_MAX);
	debug_assert(numConverted < PATH_MAX);
	char mode_c[PATH_MAX];
	numConverted = wcstombs(mode_c, mode, PATH_MAX);
	debug_assert(numConverted < PATH_MAX);

	return fopen_s(pfile, filename_c, mode_c);
}

#else

errno_t fopen_s(FILE** pfile, const char* filename, const char* mode)
{
	*pfile = NULL;
	FILE* file = fopen(filename, mode);
	if(!file)
		return ENOENT;
	*pfile = file;
	return 0;
}

#endif

#endif // #if EMULATE_SECURE_CRT
