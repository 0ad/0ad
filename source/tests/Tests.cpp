#include "ps/CLogger.h"
#include "ps/Parser.h"

#define ensure(x) if (!(x)) { ++err_count; LOG(ERROR, "", "%s:%d - test failed! (%s)", __FILE__, __LINE__, #x); }

void PerformTests()
{
	int err_count = 0;

	{
		CParser Parser;
		Parser.InputTaskType("test", "_$ident_=_$value_");

		std::string str;
		int i;

		CParserLine Line;

		ensure(Line.ParseString(Parser, "value=23"));

		ensure(Line.GetArgString(0, str) && str == "value");
		ensure(Line.GetArgInt(1, i) && i == 23);
	}


	{
		CParser Parser;
		Parser.InputTaskType("test", "_$value_[$value]_");

		std::string str;

		CParserLine Line;

		ensure(Line.ParseString(Parser, "12 34"));
		ensure(Line.GetArgCount() == 2);
		ensure(Line.GetArgString(0, str) && str == "12");
		ensure(Line.GetArgString(1, str) && str == "34");

		ensure(Line.ParseString(Parser, "56"));
		ensure(Line.GetArgCount() == 1);
		ensure(Line.GetArgString(0, str) && str == "56");

		ensure(! Line.ParseString(Parser, " "));
	}

	{
		CParser Parser;
		Parser.InputTaskType("test", "_[$value]_[$value]_[$value]_");

		std::string str;

		CParserLine Line;

		ensure(Line.ParseString(Parser, "12 34 56"));
		ensure(Line.GetArgCount() == 3);
		ensure(Line.GetArgString(0, str) && str == "12");
		ensure(Line.GetArgString(1, str) && str == "34");
		ensure(Line.GetArgString(2, str) && str == "56");

		ensure(Line.ParseString(Parser, "78 90"));
		ensure(Line.GetArgCount() == 2);
		ensure(Line.GetArgString(0, str) && str == "78");
		ensure(Line.GetArgString(1, str) && str == "90");

		ensure(Line.ParseString(Parser, "ab"));
		ensure(Line.GetArgCount() == 1);
		ensure(Line.GetArgString(0, str) && str == "ab");

		ensure(Line.ParseString(Parser, " "));
		ensure(Line.GetArgCount() == 0);
	}

	{
		CParser Parser;
		Parser.InputTaskType("test", "<[_a_][_b_]_x_>");

		std::string str;

		CParserLine Line;
		ensure(Line.ParseString(Parser, "a b x a b x"));
		ensure(Line.ParseString(Parser, "a x b x"));
		ensure(Line.ParseString(Parser, "a x"));
		ensure(Line.ParseString(Parser, "b x"));
		ensure(Line.ParseString(Parser, "x"));
		ensure(! Line.ParseString(Parser, "a x c x"));
		ensure(! Line.ParseString(Parser, "a b a x"));
		ensure(! Line.ParseString(Parser, "a"));
		ensure(! Line.ParseString(Parser, "a a x"));
		ensure(Line.ParseString(Parser, "a x a b x a x b x b x b x b x a x a x a b x a b x b x a x"));
	}

	if (err_count)
	{
		LOG(ERROR, "", "%d test failures!", err_count);
		debug_warn("Test failures!");
	}
}