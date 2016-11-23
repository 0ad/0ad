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

#include "ps/XML/XMLWriter.h"

class TestXmlWriter : public CxxTest::TestSuite
{
public:
	void test1()
	{
		XML_Start();

		{
			XML_Element("Root");
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

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"\n"
			"<Root>\n"
			"\t<!-- Comment test. -->\n"
			"\t<!-- Comment test again. -->\n"
			"\t<a one=\"1\" two=\"TWO\">b (etc)</a>\n"
			"\t<c>d</c>\n"
			"\t<c2>d2</c2>\n"
			"\t<e>\n"
			"\t\t<f>g</f>\n"
			"\t\t<h/>\n"
			"\t\t<i j=\"1.23\">\n"
			"\t\t\t<k l=\"2.34\">m</k>\n"
			"\t\t</i>\n"
			"\t</e>\n"
			"</Root>"
			);
	}

	void test_basic()
	{
		XML_Start();

		{
			XML_Element("Test");
			{
				XML_Element("example");
				{
					XML_Element("content");
					XML_Text("text");
				}
			}
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"\n"
			"<Test>\n"
			"\t<example>\n"
			"\t\t<content>text</content>\n"
			"\t</example>\n"
			"</Test>"
			);
	}

	void test_nonpretty()
	{
		XML_Start();
		XML_SetPrettyPrint(false);

		{
			XML_Element("Test");
			{
				XML_Element("example");
				{
					XML_Element("content");
					XML_Text("text");
				}
			}
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<Test><example><content>text</content></example></Test>"
			);
	}

	void test_text()
	{
		XML_Start();

		{
			XML_Element("Test");
			XML_Text("a");
			XML_Text("b");
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"\n"
			"<Test>ab</Test>"
			);
	}


	void test_utf8()
	{
		XML_Start();

		{
			XML_Element("Test");
			{
				const wchar_t text[] = { 0x0251, 0 };
				XML_Text(text);
			}
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test>\xC9\x91</Test>"
			);
	}

	void test_attr_escape()
	{
		XML_Start();

		{
			XML_Element("Test");
			XML_Attribute("example", "abc > ]]> < & \"\" ");
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test example=\"abc > ]]> &lt; &amp; &quot;&quot; \"/>"
			);
	}

	void test_chardata_escape()
	{
		XML_Start();

		{
			XML_Element("Test");
			XML_Text("abc > ]]> < & \"\" ");
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test>abc > ]]&gt; &lt; &amp; \"\" </Test>"
			);
	}

	void test_cdata_escape()
	{
		XML_Start();

		{
			XML_Element("Test");
			XML_CDATA("abc > ]]> < & \"\" ");
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test><![CDATA[abc > ]]>]]&gt;<![CDATA[ < & \"\" ]]></Test>"
			);
	}

	void test_comment_escape()
	{
		XML_Start();

		{
			XML_Element("Test");
			XML_Comment("test - -- --- ---- test");
		}

		CStr output = XML_GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test>\n"
			"\t<!-- test - \xE2\x80\x90\xE2\x80\x90 \xE2\x80\x90\xE2\x80\x90- \xE2\x80\x90\xE2\x80\x90\xE2\x80\x90\xE2\x80\x90 test -->\n"
			"</Test>"
			);
	}
};
