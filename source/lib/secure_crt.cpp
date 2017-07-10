/* Copyright (C) 2010 Wildfire Games.
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

#include "precompiled.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include "lib/secure_crt.h"

#if OS_ANDROID
# include <boost/algorithm/string/replace.hpp>
#endif

// we were included from wsecure_crt.cpp; skip all stuff that
// must only be done once.
#ifndef WSECURE_CRT
static const StatusDefinition secureCrtStatusDefinitions[] = {
	{ ERR::STRING_NOT_TERMINATED, L"Invalid string (no 0 terminator found in buffer)" }
};
STATUS_ADD_DEFINITIONS(secureCrtStatusDefinitions);
#endif


// written against http://std.dkuug.dk/jtc1/sc22/wg14/www/docs/n1031.pdf .
// optimized for size - e.g. strcpy calls strncpy with n = SIZE_MAX.

// since char and wide versions of these functions are basically the same,
// this source file implements generic versions and bridges the differences
// with these macros. wsecure_crt.cpp #defines WSECURE_CRT and
// includes this file.

// Note: These defines are all #undef:ed at the end of the file - remember to
// add a corresponding #undef when adding a #define.
#ifdef WSECURE_CRT
# define tchar wchar_t
# define tstring std::wstring
# define T(string_literal) L ## string_literal
# define tnlen wcsnlen
# define tncpy_s wcsncpy_s
# define tcpy_s wcscpy_s
# define tncat_s wcsncat_s
# define tcat_s wcscat_s
# define tcmp wcscmp
# define tcpy wcscpy
# define tvsnprintf vswprintf	// used by implementation
# define tvsprintf_s vswprintf_s
# define tsprintf_s swprintf_s
#else
# define tchar char
# define tstring std::string
# define T(string_literal) string_literal
# define tnlen strnlen
# define tncpy_s strncpy_s
# define tcpy_s strcpy_s
# define tncat_s strncat_s
# define tcat_s strcat_s
# define tcmp strcmp
# define tcpy strcpy
# define tvsnprintf vsnprintf	// used by implementation
# define tvsprintf_s vsprintf_s
# define tsprintf_s sprintf_s
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
	ENSURE(len != sizeof(char*));
*/


// skip our implementation if already available, but not the
// self-test and the t* defines (needed for test).
#if EMULATE_SECURE_CRT

#if !OS_UNIX || OS_MACOSX || OS_OPENBSD
// return length [in characters] of a string, not including the trailing
// null character. to protect against access violations, only the
// first <max_len> characters are examined; if the null character is
// not encountered by then, <max_len> is returned.
size_t tnlen(const tchar* str, size_t max_len)
{
	// note: we can't bail - what would the return value be?
	ENSURE(str != 0);

	WARN_IF_PTR_LEN(max_len);

	size_t len;
	for(len = 0; len < max_len; len++)
		if(*str++ == '\0')
			break;

	return len;
}
#endif // !OS_UNIX

#if OS_ANDROID
static tstring androidFormat(const tchar *fmt)
{
	// FIXME handle %%hs, %%ls, etc
	tstring ret(fmt);
	boost::algorithm::replace_all(ret, T("%ls"), T("%S"));
	boost::algorithm::replace_all(ret, T("%hs"), T("%s"));
	return ret;
}
#endif

// copy at most <max_src_chars> (not including trailing null) from
// <src> into <dst>, which must not overlap.
// if thereby <max_dst_chars> (including null) would be exceeded,
// <dst> is set to the empty string and ERANGE returned; otherwise,
// 0 is returned to indicate success and that <dst> is null-terminated.
//
// note: padding with zeroes is not called for by N1031.
int tncpy_s(tchar* dst, size_t max_dst_chars, const tchar* src, size_t max_src_chars)
{
	// the MS implementation returns EINVAL and allows dst = 0 if
	// max_dst_chars = max_src_chars = 0. no mention of this in
	// 3.6.2.1.1, so don't emulate that behavior.
	ENFORCE(dst != 0, ERR::INVALID_POINTER, EINVAL);
	ENFORCE(max_dst_chars != 0, ERR::INVALID_SIZE, EINVAL);	// N1031 says ERANGE, MSDN/MSVC says EINVAL
	*dst = '\0';	// in case src ENFORCE is triggered
	ENFORCE(src != 0, ERR::INVALID_POINTER, EINVAL);

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
		ENFORCE(0, ERR::INVALID_SIZE, ERANGE);
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
	ENFORCE(dst != 0, ERR::INVALID_POINTER, EINVAL);
	ENFORCE(max_dst_chars != 0, ERR::INVALID_SIZE, EINVAL);	// N1031 says ERANGE, MSDN/MSVC says EINVAL
	// src is checked in tncpy_s

	// WARN_IF_PTR_LEN not necessary: both max_dst_chars and max_src_chars
	// are checked by tnlen / tncpy_s (respectively).

	const size_t dst_len = tnlen(dst, max_dst_chars);
	if(dst_len == max_dst_chars)
	{
		*dst = '\0';
		ENFORCE(0, ERR::STRING_NOT_TERMINATED, EINVAL);	// N1031/MSDN says ERANGE, MSVC says EINVAL
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


int tvsprintf_s(tchar* dst, size_t max_dst_chars, const tchar* fmt, va_list ap)
{
	if(!dst || !fmt || max_dst_chars == 0)
	{
		errno = EINVAL;
		return -1;
	}

#if OS_ANDROID
	// Workaround for https://code.google.com/p/android/issues/detail?id=109074
	// (vswprintf doesn't null-terminate strings)
	memset(dst, 0, max_dst_chars * sizeof(tchar));

	const int ret = tvsnprintf(dst, max_dst_chars, androidFormat(fmt).c_str(), ap);
#else
	const int ret = tvsnprintf(dst, max_dst_chars, fmt, ap);
#endif
	if(ret < 0 || ret >= int(max_dst_chars))	// not enough space
	{
		dst[0] = '\0';
		return -1;
	}
	return ret;	// negative if error, else # chars written (excluding '\0')
}


int tsprintf_s(tchar* buf, size_t max_chars, const tchar* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int len = tvsprintf_s(buf, max_chars, fmt, ap);
	va_end(ap);
	return len;
}

#endif // #if EMULATE_SECURE_CRT

#undef tchar
#undef T
#undef tnlen
#undef tncpy_s
#undef tcpy_s
#undef tncat_s
#undef tcat_s
#undef tcmp
#undef tcpy
#undef tvsnprintf
#undef tvsprintf_s
#undef tsprintf_s
