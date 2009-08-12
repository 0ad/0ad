/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "ps/Parser.h"

class TestParser : public CxxTest::TestSuite 
{
public:
	void test_basic()
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


	void test_optional()
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


	void test_multi_optional()
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


	void test_optional_repeat()
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

	void test_rest()
	{
		CParser Parser;
		Parser.InputTaskType("assignment", "_$ident_=_$value[[;]$rest]");
		std::string str;

		CParserLine Line;

		TS_ASSERT(Line.ParseString(Parser, "12 = 34 ; comment"));
		TS_ASSERT_EQUALS((int)Line.GetArgCount(), 2);
		TS_ASSERT(Line.GetArgString(0, str) && str == "12");
		TS_ASSERT(Line.GetArgString(1, str) && str == "34");
	}
};
