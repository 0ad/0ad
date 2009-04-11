#include "lib/self_test.h"

#include "ps/Parser.h"

class TestParser : public CxxTest::TestSuite 
{
public:
	void test1()
	{
		CParser Parser;
		Parser.InputTaskType("test", "_$ident_=_$value_");

		std::string str;
		int i;

		CParserLine Line;

		TS_ASSERT(Line.ParseString(Parser, "value=23"));

		TS_ASSERT(Line.GetArgString(0, str) && str == "value");
		TS_ASSERT(Line.GetArgInt(1, i) && i == 23);
	}


	void test2()
	{
		CParser Parser;
		Parser.InputTaskType("test", "_$value_[$value]_");

		std::string str;

		CParserLine Line;

		TS_ASSERT(Line.ParseString(Parser, "12 34"));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 2);
		TS_ASSERT(Line.GetArgString(0, str) && str == "12");
		TS_ASSERT(Line.GetArgString(1, str) && str == "34");

		TS_ASSERT(Line.ParseString(Parser, "56"));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 1);
		TS_ASSERT(Line.GetArgString(0, str) && str == "56");

		TS_ASSERT(! Line.ParseString(Parser, " "));
	}


	void test3()
	{
		CParser Parser;
		Parser.InputTaskType("test", "_[$value]_[$value]_[$value]_");

		std::string str;

		CParserLine Line;

		TS_ASSERT(Line.ParseString(Parser, "12 34 56"));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 3);
		TS_ASSERT(Line.GetArgString(0, str) && str == "12");
		TS_ASSERT(Line.GetArgString(1, str) && str == "34");
		TS_ASSERT(Line.GetArgString(2, str) && str == "56");

		TS_ASSERT(Line.ParseString(Parser, "78 90"));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 2);
		TS_ASSERT(Line.GetArgString(0, str) && str == "78");
		TS_ASSERT(Line.GetArgString(1, str) && str == "90");

		TS_ASSERT(Line.ParseString(Parser, "ab"));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 1);
		TS_ASSERT(Line.GetArgString(0, str) && str == "ab");

		TS_ASSERT(Line.ParseString(Parser, " "));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 0);
	}


	void test4()
	{
		CParser Parser;
		Parser.InputTaskType("test", "<[_a_][_b_]_x_>");

		std::string str;

		CParserLine Line;
		TS_ASSERT(Line.ParseString(Parser, "a b x a b x"));
		TS_ASSERT(Line.ParseString(Parser, "a x b x"));
		TS_ASSERT(Line.ParseString(Parser, "a x"));
		TS_ASSERT(Line.ParseString(Parser, "b x"));
		TS_ASSERT(Line.ParseString(Parser, "x"));
		TS_ASSERT(! Line.ParseString(Parser, "a x c x"));
		TS_ASSERT(! Line.ParseString(Parser, "a b a x"));
		TS_ASSERT(! Line.ParseString(Parser, "a"));
		TS_ASSERT(! Line.ParseString(Parser, "a a x"));
		TS_ASSERT(Line.ParseString(Parser, "a x a b x a x b x b x b x b x a x a x a b x a b x b x a x"));
	}
};
