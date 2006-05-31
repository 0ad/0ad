#include <cxxtest/TestSuite.h>

#include "lib/string_s.h"

class TestString_s : public CxxTest::TestSuite 
{
	// note: avoid 4-byte strings - they would trigger WARN_IF_PTR_LEN.

	const tchar* const s0;
	const tchar* const s1;
	const tchar* const s5;
	const tchar* const s10;

	tchar d1[1];
	tchar d2[2];
	tchar d3[3];
	tchar d5[5];
	tchar d6[6];
	tchar d10[10];
	tchar d11[11];

	tchar no_null[7];


	static void TEST_LEN(const char* string, size_t limit, size_t expected)
	{
		TS_ASSERT_EQUAL(tnlen((string), (limit)), (expected));
	}

	static void TEST_CPY(char* dst, size_t dst_max, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = tcpy_s((dst), dst_max, (src));
		TS_ASSERT_EQUAL(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!tcmp(dst, T(expected_dst)));
	}

	static void TEST_CPY2(char* dst, const char* src,
		int expected_ret, const char* expected_dst)
	{
		int ret = tcpy_s((dst), ARRAY_SIZE(dst), (src));
		TS_ASSERT_EQUAL(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!tcmp(dst, T(expected_dst)));
	}

	static void TEST_NCPY(char* dst, const char* src, size_t max_src_chars,
		int expected_ret, const char* expected_dst)
	{
		int ret = tncpy_s((dst), ARRAY_SIZE(dst), (src), (max_src_chars));
		TS_ASSERT_EQUAL(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!tcmp(dst, T(expected_dst)));
	}

	static void TEST_CAT(char* dst, size_t dst_max, const char* src,
		int expected_ret, const char expected_dst)
	{
		int ret = tcat_s((dst), dst_max, (src));
		TS_ASSERT_EQUAL(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!tcmp(dst, T(expected_dst)));
	}

	static void TEST_CAT2(char* dst, const char* dst_val, const char* src,
		int expected_ret, const char* expected_dst)
	{
		tcpy(dst, T(dst_val));
		int ret = tcat_s((dst), ARRAY_SIZE(dst), (src));
		TS_ASSERT_EQUAL(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!tcmp(dst, T(expected_dst)));
	}

	static void TEST_NCAT(char* dst, const char* dst_val,
		const char* src, size_t max_src_chars,
		int expected_ret, const char* expected_dst)
	{
		tcpy(dst, T(dst_val));
		int ret = tncat_s((dst), ARRAY_SIZE(dst), (src), (max_src_chars));
		TS_ASSERT_EQUAL(ret, expected_ret);
		if(dst != 0)
			TS_ASSERT(!tcmp(dst, T(expected_dst)));
	}

public:
	TestString_s()
		: s0(T("")), s1(T("a")), s5(T("abcde")), s10(T("abcdefghij"))
	{
		const tchar no_null_[] = { 'n','o','_','n','u','l','l'};
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

		tcpy(d5, T("----"));
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
