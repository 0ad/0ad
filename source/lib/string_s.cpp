// implementation of proposed CRT safe string functions
//
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "lib.h"
#include "posix.h"	// SIZE_MAX

#include <string.h>

#ifndef PERFORM_SELF_TEST
#define PERFORM_SELF_TEST 0
#endif


// written against http://std.dkuug.dk/jtc1/sc22/wg14/www/docs/n1031.pdf .
// optimized for size - e.g. strcpy calls strncpy with n = SIZE_MAX.

// since char and wide versions of these functions are basically the same,
// this source file implements generic versions and bridges the differences
// with these macros. wstring_s.cpp #defines WSTRING_S and includes this file.
#ifdef WSTRING_S
# define tchar wchar_t
# define T(string_literal) L ## string_literal
# define tlen_s wcslen_s
# define tncpy_s wcsncpy_s
# define tcpy_s wcscpy_s
# define tncat_s wcsncat_s
# define tcat_s wcscat_s
# define tcmp wcscmp
# define tcpy wcscpy
#else
# define tchar char
# define T(string_literal) string_literal
# define tlen_s strlen_s
# define tncpy_s strncpy_s
# define tcpy_s strcpy_s
# define tncat_s strncat_s
# define tcat_s strcat_s
# define tcmp strcmp
# define tcpy strcpy
#endif


// return <retval> and raise an assertion if <condition> doesn't hold.
// usable as a statement.
#define ENFORCE(condition, retval) STMT(\
	if(!(condition))                    \
	{                                   \
		assert2(condition);             \
		return retval;                  \
	}                                   \
)

// raise a debug warning if <len> is the size of a pointer.
// catches bugs such as: tchar* s = ..; tcpy_s(s, sizeof(s), T(".."));
// if warnings get annoying, replace with debug_out. usable as a statement.
#define WARN_IF_PTR_LEN(len) STMT(                            \
	if(len == sizeof(char*))                                  \
		debug_warn("make sure string buffer size is correct");\
)


// skip the functions on VC2005 (already provided there), but not our
// self-test and the t* defines (needed for test).
#if _MSC_VER < 1400


// return length [in characters] of a string, not including the trailing
// null character. to protect against access violations, only the
// first <max_len> characters are examined; if the null character is
// not encountered by then, <max_len> is returned.
size_t tlen_s(const tchar* str, size_t max_len)
{
	// note: we can't bail - what would the return value be?
	assert2(str != 0);

	WARN_IF_PTR_LEN(max_len);

	size_t len;
	for(len = 0; len < max_len; len++)
		if(*str++ == '\0')
			break;

	return len;
}


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
	ENFORCE(dst != 0, EINVAL);
	ENFORCE(max_dst_chars != 0, ERANGE);
	*dst = '\0';	// in case src ENFORCE is triggered
	ENFORCE(src != 0, EINVAL);

	WARN_IF_PTR_LEN(max_dst_chars);
	WARN_IF_PTR_LEN(max_src_chars);

	// copy string until null character encountered or limit reached.
	// optimized for size (less comparisons than MS impl) and
	// speed (due to well-predicted jumps; we don't bother unrolling).
	tchar* p = dst;
	size_t chars_left = MIN(max_dst_chars, max_src_chars);
	while(chars_left != 0)
	{
		// success: reached end of string normally.
		if((*p++ = *src++) == '\0')
			return 0;
		chars_left--;
	}

	// which limit did we hit?
	// .. dest, and last character wasn't null: overflow.
	if(max_dst_chars <= max_src_chars)
	{
		*dst = '\0';
		ENFORCE(0 && "Buffer too small", ERANGE);
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
	ENFORCE(dst != 0, EINVAL);
	ENFORCE(max_dst_chars != 0, ERANGE);
	// src is checked in tncpy_s

	// WARN_IF_PTR_LEN not necessary: both max_dst_chars and max_src_chars
	// are checked by tlen_s / tncpy_s (respectively).

	const size_t dst_len = tlen_s(dst, max_dst_chars);
	if(dst_len == max_dst_chars)
	{
		*dst = '\0';
		ENFORCE(0 && "Destination string not null-terminated", ERANGE);
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

#endif	// #if _MSC_VER < 1400


//////////////////////////////////////////////////////////////////////////////
//
// built-in self test
//
//////////////////////////////////////////////////////////////////////////////

namespace test {

#if PERFORM_SELF_TEST

// note: avoid 4-byte strings - they would trigger WARN_IF_PTR_LEN.

static const tchar* s0 = T("");
static const tchar* s1 = T("a");
static const tchar* s5 = T("abcde");
static const tchar* s10 = T("abcdefghij");

static tchar d1[1];
static tchar d2[2];
static tchar d3[3];
static tchar d5[5];
static tchar d6[6];
static tchar d10[10];
static tchar d11[11];

static tchar no_null[] = { 'n','o','_','n','u','l','l'};



#define TEST_LEN(string, limit, expected)                                 \
STMT(                                                                     \
	assert2(tlen_s((string), (limit)) == (expected));                     \
)

#define TEST_CPY(dst, dst_max, src, expected_ret, expected_dst)           \
STMT(                                                                     \
	int ret = tcpy_s((dst), dst_max, (src));                              \
	assert2(ret == expected_ret);                                         \
	if(dst != 0)                                                          \
		assert2(!tcmp(dst, T(expected_dst)));                             \
)
#define TEST_CPY2(dst, src, expected_ret, expected_dst)                   \
STMT(                                                                     \
	int ret = tcpy_s((dst), ARRAY_SIZE(dst), (src));                      \
	assert2(ret == expected_ret);                                         \
	if(dst != 0)                                                          \
		assert2(!tcmp(dst, T(expected_dst)));                             \
)
#define TEST_NCPY(dst, src, max_src_chars, expected_ret, expected_dst)    \
STMT(                                                                     \
	int ret = tncpy_s((dst), ARRAY_SIZE(dst), (src), (max_src_chars));    \
	assert2(ret == expected_ret);                                         \
	if(dst != 0)                                                          \
		assert2(!tcmp(dst, T(expected_dst)));                             \
)

#define TEST_CAT(dst, dst_max, src, expected_ret, expected_dst)           \
STMT(                                                                     \
	int ret = tcat_s((dst), dst_max, (src));                              \
	assert2(ret == expected_ret);                                         \
	if(dst != 0)                                                          \
		assert2(!tcmp(dst, T(expected_dst)));                             \
)
#define TEST_CAT2(dst, dst_val, src, expected_ret, expected_dst)          \
STMT(                                                                     \
	tcpy(dst, T(dst_val));                                                \
	int ret = tcat_s((dst), ARRAY_SIZE(dst), (src));                      \
	assert2(ret == expected_ret);                                         \
	if(dst != 0)                                                          \
		assert2(!tcmp(dst, T(expected_dst)));                             \
)
#define TEST_NCAT(dst, dst_val, src, max_src_chars, expected_ret, expected_dst)\
STMT(                                                                     \
	tcpy(dst, T(dst_val));                                                \
	int ret = tncat_s((dst), ARRAY_SIZE(dst), (src), (max_src_chars));    \
	assert2(ret == expected_ret);                                         \
	if(dst != 0)                                                          \
		assert2(!tcmp(dst, T(expected_dst)));                             \
)


static void test_length()
{
	TEST_LEN(s0, 0 , 0 );
	TEST_LEN(s0, 1 , 0 );
	TEST_LEN(s0, 50, 0 );
	TEST_LEN(s1, 0 , 0 );
	TEST_LEN(s1, 1 , 1 );
	TEST_LEN(s1, 50, 1 );
	TEST_LEN(s5, 0 , 0 );
	TEST_LEN(s5, 1 , 1 );
	TEST_LEN(s5, 50, 5 );
	TEST_LEN(s10,9 , 9 );
	TEST_LEN(s10,10, 10);
	TEST_LEN(s10,11, 10);
}


static void test_copy()
{
	TEST_CPY(0 ,0,0 , EINVAL,"");	// all invalid
	TEST_CPY(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
	TEST_CPY(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
	TEST_CPY(d1,1,0 , EINVAL,"");	// src = 0
	TEST_CPY(d1,0,s1, ERANGE,"");	// max_dst_chars = 0

	TEST_CPY2(d1 ,s1, ERANGE,"");
	TEST_CPY2(d1 ,s5, ERANGE,"");
	TEST_CPY2(d5 ,s5, ERANGE,"");
	TEST_CPY2(d2 ,s1, 0,"a");
	TEST_CPY2(d6 ,s5, 0,"abcde");
	TEST_CPY2(d11,s5, 0,"abcde");

	TEST_NCPY(d1 ,s1,1, ERANGE,"");
	TEST_NCPY(d1 ,s5,1, ERANGE,"");
	TEST_NCPY(d5 ,s5,5, ERANGE,"");
	TEST_NCPY(d2 ,s1,1, 0,"a");
	TEST_NCPY(d6 ,s5,5, 0,"abcde");
	TEST_NCPY(d11,s5,5, 0,"abcde");

	tcpy(d5, T("----"));
	TEST_NCPY(d5,s5,0 , 0,"");	// specified behavior! see 3.6.2.1.1 #4
	TEST_NCPY(d5,s5,1 , 0,"a");
	TEST_NCPY(d5,s5,4 , 0,"abcd");
	TEST_NCPY(d6,s5,5 , 0,"abcde");
	TEST_NCPY(d6,s5,10, 0,"abcde");
}


static void test_concatenate()
{
	TEST_CAT(0 ,0,0 , EINVAL,"");	// all invalid
	TEST_CAT(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
	TEST_CAT(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
	TEST_CAT(d1,1,0 , EINVAL,"");	// src = 0
	TEST_CAT(d1,0,s1, ERANGE,"");	// max_dst_chars = 0
	TEST_CAT(no_null,5,s1, ERANGE,"");	// dst not terminated

	TEST_CAT2(d1 ,"" ,s1, ERANGE,"");
	TEST_CAT2(d1 ,"" ,s5, ERANGE,"");
	TEST_CAT2(d3 ,"1",s1, 0,"1a");
	TEST_CAT2(d5 ,"1",s1, 0,"1a");
	TEST_CAT2(d6 ,"" ,s5, 0,"abcde");
	TEST_CAT2(d10,"" ,s5, 0,"abcde");
	TEST_CAT2(d10,"" ,s10, ERANGE,"");		// empty, total overflow
	TEST_CAT2(d10,"12345",s5 , ERANGE,"");	// not empty, overflow
	TEST_CAT2(d10,"12345",s10, ERANGE,"");	// not empty, total overflow
	TEST_CAT2(d10,"1234" ,s5 , 0,"1234abcde");

	TEST_NCAT(d1 ,"" ,s1,1, ERANGE,"");
	TEST_NCAT(d1 ,"" ,s5,5, ERANGE,"");
	TEST_NCAT(d3 ,"1",s1,1, 0,"1a");
	TEST_NCAT(d5 ,"1",s1,1, 0,"1a");
	TEST_NCAT(d6 ,"" ,s5,5, 0,"abcde");
	TEST_NCAT(d10,"" ,s5,5, 0,"abcde");
	TEST_NCAT(d10,"" ,s10,10, ERANGE,"");		// empty, total overflow
	TEST_NCAT(d10,"12345",s5 ,5 , ERANGE,"");	// not empty, overflow
	TEST_NCAT(d10,"12345",s10,10, ERANGE,"");	// not empty, total overflow
	TEST_NCAT(d10,"1234" ,s5 ,5 , 0,"1234abcde");

	TEST_NCAT(d5,"----",s5,0 , 0,"----");
	TEST_NCAT(d5,"",s5,1 , 0,"a");
	TEST_NCAT(d5,"",s5,4 , 0,"abcd");
	TEST_NCAT(d5,"12",s5,2 , 0,"12ab");
	TEST_NCAT(d6,"",s5,10, 0,"abcde");
}


static int run_tests()
{
	test_length();
	test_copy();
	test_concatenate();
	return 0;
}

static int dummy = run_tests();

#endif	// #if PERFORM_SELF_TEST

}	// namespace test
