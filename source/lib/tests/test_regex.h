#include "lib/self_test.h"

#include "lib/regex.h"

class TestRegex : public CxxTest::TestSuite 
{
public:
	void test_regex()
	{
		TS_ASSERT_EQUALS(match_wildcard("", ""), 1);
		TS_ASSERT_EQUALS(match_wildcard("a", 0), 1);	// NULL matches everything

		TS_ASSERT_EQUALS(match_wildcard("abc", "abc")     , 1);	// direct match
		TS_ASSERT_EQUALS(match_wildcard("abc", "???")     , 1);	// only ?
		TS_ASSERT_EQUALS(match_wildcard("abc", "*"  )     , 1);	// only *

		TS_ASSERT_EQUALS(match_wildcard("ab" , "a?" )     , 1);	// trailing ?
		TS_ASSERT_EQUALS(match_wildcard("abc", "a?c")     , 1);	// middle ?
		TS_ASSERT_EQUALS(match_wildcard("abc", "?bc")     , 1);	// leading ?

		TS_ASSERT_EQUALS(match_wildcard("abc", "a*" )     , 1);	// trailing *
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "ab*ef"), 1);	// middle *
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "*f"   ), 1);	// leading *

		TS_ASSERT_EQUALS(match_wildcard("abcdef", "a?cd*"), 1);	// ? and *
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "a*d?f"), 1);	// * and ?
		TS_ASSERT_EQUALS(match_wildcard("abcdef", "a*d*" ), 1);	// multiple *

		// unicode test pasted from the above; keep in sync!

		TS_ASSERT_EQUALS(match_wildcardw(L"", L""), 1);
		TS_ASSERT_EQUALS(match_wildcardw(L"a", 0), 1);	// NULL matches everything

		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"abc")     , 1);	// direct match
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"???")     , 1);	// only ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"*"  )     , 1);	// only *

		TS_ASSERT_EQUALS(match_wildcardw(L"ab" , L"a?" )     , 1);	// trailing ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"a?c")     , 1);	// middle ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"?bc")     , 1);	// leading ?

		TS_ASSERT_EQUALS(match_wildcardw(L"abc", L"a*" )     , 1);	// trailing *
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"ab*ef"), 1);	// middle *
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"*f"   ), 1);	// leading *

		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"a?cd*"), 1);	// ? and *
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"a*d?f"), 1);	// * and ?
		TS_ASSERT_EQUALS(match_wildcardw(L"abcdef", L"a*d*" ), 1);	// multiple *
	}
};
