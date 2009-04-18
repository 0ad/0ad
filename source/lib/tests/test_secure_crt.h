#include "lib/self_test.h"

#include "lib/secure_crt.h"

// note: we only test the char version. this avoids having to
// expose secure_crt.cpp's tchar / tcpy etc. macros in the header and/or
// writing a copy of this test for the unicode version.
// secure_crt.cpp's unicode functions are the same anyway
// (they're implemented via the abovementioned tcpy macro redirection).

class TestString_s : public CxxTest::TestSuite 
{
	// note: avoid 4-byte strings - they would trigger WARN::IF_PTR_LEN.

	const char* const s0;
	const char* const s1;
	const char* const s5;
	const char* const s10;

	char d1[1];
	char d2[2];
	char d3[3];
	char d5[5];
	char d6[6];
	char d10[10];
	char d11[11];

	char no_null[7];


	static void TEST_LEN(const char* string, size_t limit,
		size_t expected_len)
	{
		TS_ASSERT_EQUALS(strnlen((string), (limit)), (expected_len));
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
		: s0(""), s1("a"), s5("abcde"), s10("abcdefghij")
	{
		const char no_null_tmp[] = { 'n','o','_','n','u','l','l'};
		memcpy(no_null, no_null_tmp, sizeof(no_null));
	}

	// contains all tests that verify correct behavior for bogus input.
	// our implementation suppresses error dialogs while the self-test is active,
	// but others (e.g. the functions shipped with VC8) do not.
	// since we have no control over their error reporting (which ends up taking
	// down the program), we must skip this part of the test if using them.
	// this is still preferable to completely disabling the self-test.
	void test_param_validation()
	{
	#if EMULATE_SECURE_CRT
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CPY(0 ,0,0 , EINVAL,"");	// all invalid
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CPY(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CPY(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CPY(d1,1,0 , EINVAL,"");	// src = 0
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CPY(d1,0,s1, ERANGE,"");	// max_dst_chars = 0

		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CPY2(d1,1, s1, ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CPY2(d1,1, s5, ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CPY2(d5,5, s5, ERANGE,"");

		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCPY(d1,1 ,s1,1, ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCPY(d1,1 ,s5,1, ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCPY(d5,5 ,s5,5, ERANGE,"");

		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CAT(0 ,0,0 , EINVAL,"");	// all invalid
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CAT(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CAT(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CAT(d1,1,0 , EINVAL,"");	// src = 0
		debug_SkipNextError(ERR::INVALID_PARAM);
		TEST_CAT(d1,0,s1, ERANGE,"");	// max_dst_chars = 0
		debug_SkipNextError(ERR::STRING_NOT_TERMINATED);
		TEST_CAT(no_null,5,s1, ERANGE,"");	// dst not terminated

		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CAT2(d1,1, s1, "",ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CAT2(d1,1, s5, "",ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CAT2(d10,10, s10, "",ERANGE,"");		// empty, total overflow
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CAT2(d10,10, s5, "12345",ERANGE,"");	// not empty, overflow
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_CAT2(d10,10, s10, "12345",ERANGE,"");	// not empty, total overflow

		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCAT(d1,1, s1,1, "",ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCAT(d1,1, s5,5, "",ERANGE,"");
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCAT(d10,10, s10,10, "",ERANGE,"");		// empty, total overflow
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCAT(d10,10, s5,5, "12345",ERANGE,"");		// not empty, overflow
		debug_SkipNextError(ERR::BUF_SIZE);
		TEST_NCAT(d10,10, s10,10, "12345",ERANGE,"");	// not empty, total overflow
	#endif
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
};
