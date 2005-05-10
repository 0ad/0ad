#include "precompiled.h"

#include "ps/CLogger.h"
#include "ps/Parser.h"
#include "ps/XMLWriter.h"

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



	{
		XML_Start("utf-8");
		XML_Doctype("Scenario", "/maps/scenario.dtd");

		{
			XML_Element("Scenario");
			{
				XML_Comment("Comment test.");
				XML_Comment("Comment test again.");
				{
					XML_Element("a");
					XML_Attribute("one", 1);
					XML_Attribute("two", "TWO");
					XML_Text("b");
					XML_Text(" (etc)");
				}
				{
					XML_Element("c");
					XML_Text("d");
				}
				XML_Setting("c2", "d2");
				{
					XML_Element("e");
					{
						{
							XML_Element("f");
							XML_Text("g");
						}
						{
							XML_Element("h");
						}
						{
							XML_Element("i");
							XML_Attribute("j", 1.23);
							{
								XML_Element("k");
								XML_Attribute("l", 2.34);
								XML_Text("m");
							}
						}
					}
				}
			}
		}

		// For this test to be useful, it should actually test something.
	}

	{
 		const wchar_t chr_utf16[] = { 0x12, 0xff, 0x1234, 0x3456, 0x5678, 0x7890, 0x9abc, 0xbcde, 0xfffe };
		const unsigned char chr_utf8[] = { 0x12, 0xc3, 0xbf, 0xe1, 0x88, 0xb4, 0xe3, 0x91, 0x96, 0xe5, 0x99, 0xb8, 0xe7, 0xa2, 0x90, 0xe9, 0xaa, 0xbc, 0xeb, 0xb3, 0x9e, 0xef, 0xbf, 0xbe };
		CStrW str_utf16 (chr_utf16, sizeof(chr_utf16)/sizeof(wchar_t));
		CStr8 str_utf8 = str_utf16.ToUTF8();
		ensure(str_utf8.length() == sizeof(chr_utf8));
		ensure(memcmp(str_utf8.data(), chr_utf8, sizeof(chr_utf8)) == 0);
		ensure(str_utf8.FromUTF8() == str_utf16);
	}
}