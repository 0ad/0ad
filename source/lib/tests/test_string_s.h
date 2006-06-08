#include "lib/self_test.h"

#include "lib/lib.h"
#include "lib/string_s.h"

// note: we only test the char version. this avoids having to
// expose string_s.cpp's tchar / tcpy etc. macros in the header and/or
// writing a copy of this test for the unicode version.
// string_s.cpp's unicode functions are the same anyway
// (they're implemented via the abovementioned tcpy macro redirection).

class TestString_s : public CxxTest::TestSuite 
{
	// note: avoid 4-byte strings - they would trigger WARN_IF_PTR_LEN.

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


	static void TEST_LEN(const char* string, size_t limit, size_t expected)
	{
		TS_ASSERT_EQUALS(strnlen((string), (limit)), (expected));
	}

	static void TEST_CPY(char* dst, size_t dst_max, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = strcpy_s((dst), dst_max, (src));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_CPY2(char* dst, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = strcpy_s((dst), ARRAY_SIZE(dst), (src));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_NCPY(char* dst, const char* src, size_t max_src_chars,
		int expected_ret, const char* expected_dst)
	{
		int ret = strncpy_s((dst), ARRAY_SIZE(dst), (src), (max_src_chars));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_CAT(char* dst, size_t dst_max, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = strcat_s((dst), dst_max, (src));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_CAT2(char* dst, const char* dst_val, const char* src,
		int expected_ret, const char* expected_dst)
	{
		strcpy(dst, dst_val);
		int ret = strcat_s((dst), ARRAY_SIZE(dst), (src));
		TS_ASSERT_EQUALS(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!strcmp(dst, expected_dst));
	}

	static void TEST_NCAT(char* dst, const char* dst_val,
		const char* src, size_t max_src_chars,
		int expected_ret, const char* expected_dst)
	{
		strcpy(dst, dst_val);
		int ret = strncat_s((dst), ARRAY_SIZE(dst), (src), (max_src_chars));
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
	#if !HAVE_STRING_S
		TEST_CPY(0 ,0,0 , EINVAL,"");	// all invalid
		TEST_CPY(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
		TEST_CPY(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
		TEST_CPY(d1,1,0 , EINVAL,"");	// src = 0
		TEST_CPY(d1,0,s1, ERANGE,"");	// max_dst_chars = 0

		TEST_CPY2(d1 ,s1, ERANGE,"");
		TEST_CPY2(d1 ,s5, ERANGE,"");
		TEST_CPY2(d5 ,s5, ERANGE,"");

		TEST_NCPY(d1 ,s1,1, ERANGE,"");
		TEST_NCPY(d1 ,s5,1, ERANGE,"");
		TEST_NCPY(d5 ,s5,5, ERANGE,"");

		TEST_CAT(0 ,0,0 , EINVAL,"");	// all invalid
		TEST_CAT(0 ,0,s1, EINVAL,"");	// dst = 0, max = 0
		TEST_CAT(0 ,1,s1, EINVAL,"");	// dst = 0, max > 0
		TEST_CAT(d1,1,0 , EINVAL,"");	// src = 0
		TEST_CAT(d1,0,s1, ERANGE,"");	// max_dst_chars = 0
		TEST_CAT(no_null,5,s1, ERANGE,"");	// dst not terminated

		TEST_CAT2(d1 ,"" ,s1, ERANGE,"");
		TEST_CAT2(d1 ,"" ,s5, ERANGE,"");
		TEST_CAT2(d10,"" ,s10, ERANGE,"");		// empty, total overflow
		TEST_CAT2(d10,"12345",s5 , ERANGE,"");	// not empty, overflow
		TEST_CAT2(d10,"12345",s10, ERANGE,"");	// not empty, total overflow

		TEST_NCAT(d1 ,"" ,s1,1, ERANGE,"");
		TEST_NCAT(d1 ,"" ,s5,5, ERANGE,"");
		TEST_NCAT(d10,"" ,s10,10, ERANGE,"");		// empty, total overflow
		TEST_NCAT(d10,"12345",s5 ,5 , ERANGE,"");	// not empty, overflow
		TEST_NCAT(d10,"12345",s10,10, ERANGE,"");	// not empty, total overflow
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
		TEST_CPY2(d2 ,s1, 0,"a");
		TEST_CPY2(d6 ,s5, 0,"abcde");
		TEST_CPY2(d11,s5, 0,"abcde");

		TEST_NCPY(d2 ,s1,1, 0,"a");
		TEST_NCPY(d6 ,s5,5, 0,"abcde");
		TEST_NCPY(d11,s5,5, 0,"abcde");

		strcpy(d5, "----");
		TEST_NCPY(d5,s5,0 , 0,"");	// specified behavior! see 3.6.2.1.1 #4
		TEST_NCPY(d5,s5,1 , 0,"a");
		TEST_NCPY(d5,s5,4 , 0,"abcd");
		TEST_NCPY(d6,s5,5 , 0,"abcde");
		TEST_NCPY(d6,s5,10, 0,"abcde");
	}


	void test_concatenate()
	{
		TEST_CAT2(d3 ,"1",s1, 0,"1a");
		TEST_CAT2(d5 ,"1",s1, 0,"1a");
		TEST_CAT2(d6 ,"" ,s5, 0,"abcde");
		TEST_CAT2(d10,"" ,s5, 0,"abcde");
		TEST_CAT2(d10,"1234" ,s5 , 0,"1234abcde");

		TEST_NCAT(d3 ,"1",s1,1, 0,"1a");
		TEST_NCAT(d5 ,"1",s1,1, 0,"1a");
		TEST_NCAT(d6 ,"" ,s5,5, 0,"abcde");
		TEST_NCAT(d10,"" ,s5,5, 0,"abcde");
		TEST_NCAT(d10,"1234" ,s5 ,5 , 0,"1234abcde");

		TEST_NCAT(d5,"----",s5,0 , 0,"----");
		TEST_NCAT(d5,"",s5,1 , 0,"a");
		TEST_NCAT(d5,"",s5,4 , 0,"abcd");
		TEST_NCAT(d5,"12",s5,2 , 0,"12ab");
		TEST_NCAT(d6,"",s5,10, 0,"abcde");
	}
};
