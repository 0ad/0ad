#include "lib/self_test.h"

#include "ps/GameSetup/CmdLineArgs.h"

class TestCmdLineArgs : public CxxTest::TestSuite 
{
public:
	void test_has()
	{
		const char* argv[] = { "program", "-test2" };
		CmdLineArgs c(ARRAY_SIZE(argv), argv);
		TS_ASSERT(!c.Has("test1"));
		TS_ASSERT(c.Has("test2"));
		TS_ASSERT(!c.Has("test3"));
		TS_ASSERT(!c.Has(""));
	}

	void test_get()
	{
		const char* argv[] = { "program", "-test1=", "-test2=x", "-test3=-y=y-", "-=z" };
		CmdLineArgs c(ARRAY_SIZE(argv), argv);
		TS_ASSERT_STR_EQUALS(c.Get("test0"), "");
		TS_ASSERT_STR_EQUALS(c.Get("test1"), "");
		TS_ASSERT_STR_EQUALS(c.Get("test2"), "x");
		TS_ASSERT_STR_EQUALS(c.Get("test3"), "-y=y-");
		TS_ASSERT_STR_EQUALS(c.Get(""), "z");
	}

	void test_multiple()
	{
		const char* argv[] = { "program", "-test1=one", "-test1=two", "-test2=none", "-test1=three" };
		CmdLineArgs c(ARRAY_SIZE(argv), argv);

		TS_ASSERT_STR_EQUALS(c.Get("test1"), "one");
		TS_ASSERT_STR_EQUALS(c.Get("test2"), "none");

		std::vector<CStr> test1 = c.GetMultiple("test1");
		CStr expected1[] = { "one", "two", "three" };
		TS_ASSERT_VECTOR_EQUALS_ARRAY(test1, expected1);

		std::vector<CStr> test2 = c.GetMultiple("test2");
		CStr expected2[] = { "none" };
		TS_ASSERT_VECTOR_EQUALS_ARRAY(test2, expected2);

		std::vector<CStr> test3 = c.GetMultiple("test3");
		TS_ASSERT_EQUALS(test3.size(), (size_t)0);
	}

	void test_get_invalid()
	{
		const char* argv[] = { "-test1", "-test2", "test3", " -test4" };
		CmdLineArgs c(ARRAY_SIZE(argv), argv);

		TS_ASSERT(!c.Has("test1"));
		TS_ASSERT(c.Has("test2"));
		TS_ASSERT(!c.Has("test3"));
		TS_ASSERT(!c.Has("test4"));
	}

	void test_arg0()
	{
		const char* argv[] = { "program" };
		CmdLineArgs c(ARRAY_SIZE(argv), argv);
		TS_ASSERT_STR_EQUALS(c.GetArg0(), "program");

		CmdLineArgs c2(0, NULL);
		TS_ASSERT_STR_EQUALS(c2.GetArg0(), "");
	}
};
