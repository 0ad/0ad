/* Copyright (C) 2016 Wildfire Games.
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

#include "lib/self_test.h"

#include "lib/secure_crt.h"

// note: we only test the char version. this avoids having to
// expose secure_crt.cpp's tchar / tcpy etc. macros in the header and/or
// writing a copy of this test for the unicode version.
// secure_crt.cpp's unicode functions are the same anyway
// (they're implemented via the abovementioned tcpy macro redirection).


#if OS_WIN
// helper class to disable CRT error dialogs
class SuppressErrors
{
public:
	SuppressErrors()
	{
		// Redirect the assertion output to somewhere where it shouldn't be noticed
		old_mode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
		// Replace the invalid parameter handler with one that ignores everything
		old_handler = _set_invalid_parameter_handler(&invalid_parameter_handler);
	}
	~SuppressErrors()
	{
		_CrtSetReportMode(_CRT_ASSERT, old_mode);
		_set_invalid_parameter_handler(old_handler);
	}

private:
	int old_mode;
	_invalid_parameter_handler old_handler;

	static void invalid_parameter_handler(const wchar_t* UNUSED(expression), const wchar_t* UNUSED(function),
		const wchar_t* UNUSED(file), unsigned int UNUSED(line), uintptr_t UNUSED(pReserved))
	{
		return;
	}
};
#else
class SuppressErrors
{
public:
	SuppressErrors()
	{
	}
};
#endif


class TestString_s : public CxxTest::TestSuite
{
	// note: avoid 4-byte strings - they would trigger WARN::IF_PTR_LEN.

	const char* const s0;
	const char* const s1;
	const char* const s5;
	const char* const s10;
	const wchar_t* const ws10;

	char d1[1];
	char d2[2];
	char d3[3];
	char d5[5];
	char d6[6];
	char d10[10];
	wchar_t wd10[10];
	char d11[11];

	char no_null[7];


	static void TEST_LEN(const char* string, size_t limit,
		size_t expected_len)
	{
		TS_ASSERT_EQUALS(int(strnlen((string), int(limit))), int(expected_len));
	}

	static void TEST_CPY(char* dst, size_t dst_max, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = strcpy_s(dst, dst_max, src);
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_CPY2(char* dst, size_t max_dst_chars, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = strcpy_s((dst), max_dst_chars, (src));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_NCPY(char* dst, size_t max_dst_chars, const char* src, size_t max_src_chars,
		int expected_ret, const char* expected_dst)
	{
		int ret = strncpy_s(dst, max_dst_chars, src, max_src_chars);
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_CAT(char* dst, size_t max_dst_chars, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = strcat_s(dst, max_dst_chars, src);
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_CAT2(char* dst, size_t max_dst_chars, const char* src,
		const char* dst_val, int expected_ret, const char* expected_dst)
	{
		strcpy(dst, dst_val);
		int ret = strcat_s(dst, max_dst_chars, src);
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_NCAT(char* dst, size_t max_dst_chars, const char* src, size_t max_src_chars,
		const char* dst_val, int expected_ret, const char* expected_dst)
	{
		strcpy(dst, dst_val);
		int ret = strncat_s(dst, max_dst_chars, src, (max_src_chars));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

public:
	TestString_s()
		: s0(""), s1("a"), s5("abcde"), s10("abcdefghij"), ws10(L"abcdefghij")
	{
		const char no_null_tmp[] = { 'n','o','_','n','u','l','l'};
		memcpy(no_null, no_null_tmp, sizeof(no_null));
	}

	// contains all tests that verify correct behavior for bogus input.
	// our implementation suppresses error dialogs while the self-test is active,
	// but others (e.g. the functions shipped with VC8) need the code in
	// SuppressErrors to disable the error dialogs.
	void test_param_validation()
	{
		SuppressErrors suppress;

#if EMULATE_SECURE_CRT
# define SKIP_ERRORS(err) debug_SkipErrors(err)
# define STOP_SKIPPING_ERRORS(expectedCount) TS_ASSERT_EQUALS(debug_StopSkippingErrors(), (size_t)expectedCount)
#else
# define SKIP_ERRORS(err) (void)0
# define STOP_SKIPPING_ERRORS(expectedCount) (void)0
#endif

		SKIP_ERRORS(ERR::INVALID_POINTER);
		TEST_CPY(0 ,0,0 , EINVAL,"");	// all invalid
		TEST_CPY(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
		TEST_CPY(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
		TEST_CPY(d1,1,0 , EINVAL,"");	// src = 0
		STOP_SKIPPING_ERRORS(4);

		SKIP_ERRORS(ERR::INVALID_SIZE);
		TEST_CPY(d1,0,s1, EINVAL,"");	// max_dst_chars = 0
		TEST_CPY2(d1,1, s1, ERANGE,"");
		TEST_CPY2(d1,1, s5, ERANGE,"");
		TEST_CPY2(d5,5, s5, ERANGE,"");
		STOP_SKIPPING_ERRORS(4);

		SKIP_ERRORS(ERR::INVALID_SIZE);
		TEST_NCPY(d1,1 ,s1,1, ERANGE,"");
		TEST_NCPY(d1,1 ,s5,1, ERANGE,"");
		TEST_NCPY(d5,5 ,s5,5, ERANGE,"");
		STOP_SKIPPING_ERRORS(3);

		SKIP_ERRORS(ERR::INVALID_POINTER);
		TEST_CAT(0 ,0,0 , EINVAL,"");	// all invalid
		TEST_CAT(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
		TEST_CAT(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
		TEST_CAT(d1,1,0 , EINVAL,"");	// src = 0
		STOP_SKIPPING_ERRORS(4);
		SKIP_ERRORS(ERR::INVALID_SIZE);
		TEST_CAT(d1,0,s1, EINVAL,"");	// max_dst_chars = 0
		STOP_SKIPPING_ERRORS(1);

		SKIP_ERRORS(ERR::STRING_NOT_TERMINATED);
		TEST_CAT(no_null,5,s1, EINVAL,"");	// dst not terminated
		STOP_SKIPPING_ERRORS(1);

		SKIP_ERRORS(ERR::INVALID_SIZE);
		TEST_CAT2(d1,1, s1, "",ERANGE,"");
		TEST_CAT2(d1,1, s5, "",ERANGE,"");
		TEST_CAT2(d10,10, s10, "",ERANGE,"");		// empty, total overflow
		TEST_CAT2(d10,10, s5, "12345",ERANGE,"");	// not empty, overflow
		TEST_CAT2(d10,10, s10, "12345",ERANGE,"");	// not empty, total overflow
		STOP_SKIPPING_ERRORS(5);

		SKIP_ERRORS(ERR::INVALID_SIZE);
		TEST_NCAT(d1,1, s1,1, "",ERANGE,"");
		TEST_NCAT(d1,1, s5,5, "",ERANGE,"");
		TEST_NCAT(d10,10, s10,10, "",ERANGE,"");		// empty, total overflow
		TEST_NCAT(d10,10, s5,5, "12345",ERANGE,"");		// not empty, overflow
		TEST_NCAT(d10,10, s10,10, "12345",ERANGE,"");	// not empty, total overflow
		STOP_SKIPPING_ERRORS(5);

#undef SKIP_ERRORS
#undef STOP_SKIPPING_ERRORS
	}


	void test_length()
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


	void test_copy()
	{
		TEST_CPY2(d2,2 ,s1, 0,"a");
		TEST_CPY2(d6,6 ,s5, 0,"abcde");
		TEST_CPY2(d11,11, s5, 0,"abcde");

		TEST_NCPY(d2,2 ,s1,1, 0,"a");
		TEST_NCPY(d6,6 ,s5,5, 0,"abcde");
		TEST_NCPY(d11,11, s5,5, 0,"abcde");

		strcpy(d5, "----");
		TEST_NCPY(d5,5, s5,0 , 0,"");	// specified behavior! see 3.6.2.1.1 #4
		TEST_NCPY(d5,5, s5,1 , 0,"a");
		TEST_NCPY(d6,6, s5,5 , 0,"abcde");
		TEST_NCPY(d6,6, s5,10, 0,"abcde");
	}


	void test_concatenate()
	{
		TEST_CAT2(d3,3, s1, "1",0,"1a");
		TEST_CAT2(d5,5, s1, "1",0,"1a");
		TEST_CAT2(d6,6, s5, "",0,"abcde");
		TEST_CAT2(d10,10, s5, "",0,"abcde");
		TEST_CAT2(d10,10, s5, "1234",0,"1234abcde");

		TEST_NCAT(d3,3, s1,1, "1",0,"1a");
		TEST_NCAT(d5,5, s1,1, "1",0,"1a");
		TEST_NCAT(d6,6, s5,5, "",0,"abcde");
		TEST_NCAT(d10,10, s5,5, "",0,"abcde");
		TEST_NCAT(d10,10, s5,5, "1234",0,"1234abcde");

		TEST_NCAT(d5,5, s5,0, "----",0,"----");
		TEST_NCAT(d5,5, s5,1, "",0,"a");
		TEST_NCAT(d5,5, s5,4, "",0,"abcd");
		TEST_NCAT(d5,5, s5,2, "12",0,"12ab");
		TEST_NCAT(d6,6, s5,10, "",0,"abcde");
	}


	static void TEST_PRINTF(char* dst, size_t max_dst_chars, const char* dst_val,
		int expected_ret, const char* expected_dst, const char* fmt, ...)
	{
		if (dst)
			strcpy(dst, dst_val);
		va_list ap;
		va_start(ap, fmt);
		int ret = vsprintf_s(dst, max_dst_chars, fmt, ap);
		va_end(ap);
		TS_ASSERT_EQUALS(ret, expected_ret);
		if (dst)
			TS_ASSERT_STR_EQUALS(dst, expected_dst);
	}

	static void TEST_WPRINTF(wchar_t* dst, size_t max_dst_chars, const wchar_t* dst_val,
		int expected_ret, const wchar_t* expected_dst, const wchar_t* fmt, ...)
	{
		if (dst)
			wcscpy(dst, dst_val);
		va_list ap;
		va_start(ap, fmt);
		int ret = vswprintf_s(dst, max_dst_chars, fmt, ap);
		va_end(ap);
		TS_ASSERT_EQUALS(ret, expected_ret);
		if (dst)
			TS_ASSERT_WSTR_EQUALS(dst, expected_dst);
	}

	void test_printf_overflow()
	{
		TEST_PRINTF(d10,10, s10, 4, "1234", "%d", 1234);
		TEST_PRINTF(d10,5, s10, 4, "1234", "%d", 1234);

		SuppressErrors suppress;
		TEST_PRINTF(d10,4, s10, -1, "", "%d", 1234);
		TEST_PRINTF(d10,3, s10, -1, "", "%d", 1234);
		TEST_PRINTF(d10,0, s10, -1, "abcdefghij", "%d", 1234);
		TEST_PRINTF(NULL,0, NULL, -1, "", "%d", 1234);
		TEST_PRINTF(d10,10, s10, -1, "abcdefghij", NULL);
	}

	void test_wprintf_overflow()
	{
		TEST_WPRINTF(wd10,10, ws10, 4, L"1234", L"%d", 1234);
		TEST_WPRINTF(wd10,5, ws10, 4, L"1234", L"%d", 1234);

		SuppressErrors suppress;
		TEST_WPRINTF(wd10,4, ws10, -1, L"", L"%d", 1234);
		TEST_WPRINTF(wd10,3, ws10, -1, L"", L"%d", 1234);
		TEST_WPRINTF(wd10,0, ws10, -1, L"abcdefghij", L"%d", 1234);
		TEST_WPRINTF(NULL,0, NULL, -1, L"", L"%d", 1234);
		TEST_WPRINTF(wd10,10, ws10, -1, L"abcdefghij", NULL);
	}

	void test_printf_strings()
	{
		TEST_PRINTF(d10,10, s10, 3, "123", "%s", "123");
		TEST_PRINTF(d10,10, s10, 3, "123", "%hs", "123");
		TEST_PRINTF(d10,10, s10, 3, "123", "%ls", L"123");
	}

	void test_wprintf_strings()
	{
		TEST_WPRINTF(wd10,10, ws10, 3, L"123", L"%hs", "123");
		TEST_WPRINTF(wd10,10, ws10, 3, L"123", L"%ls", L"123");
	}
};
