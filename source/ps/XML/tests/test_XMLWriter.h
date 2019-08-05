/* Copyright (C) 2019 Wildfire Games.
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
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Root");
			{
				testFile.Comment("Comment test.");
				testFile.Comment("Comment test again.");
				{
					XMLWriter_Element testTag1(testFile, "a");
					testTag1.Attribute("one", 1);
					testTag1.Attribute("two", "TWO");
					testTag1.Text("b", false);
					testTag1.Text(" (etc)", false);
				}
				{
					XMLWriter_Element testTag2(testFile, "c");
					testTag2.Text("d", false);
				}
				rootTag.Setting("c2", "d2");
				{
					XMLWriter_Element testTag3(testFile, "e");
					{
						{
							XMLWriter_Element testTag4(testFile, "f");
							testTag4.Text("g", false);
						}
						{
							XMLWriter_Element testTag5(testFile, "h");
						}
						{
							XMLWriter_Element testTag6(testFile, "i");
							testTag6.Attribute("j", 1.23);
							{
								XMLWriter_Element testTag7(testFile, "k");
								testTag7.Attribute("l", 2.34);
								testTag7.Text("m", false);
							}
						}
					}
				}
			}
		}

		CStr output = testFile.GetOutput();
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
		XMLWriter_File testFile;
		{
			XMLWriter_Element testTag1(testFile, "Test");
			{
				XMLWriter_Element testTag2(testFile, "example");
				{
					XMLWriter_Element testTag3(testFile, "content");
					testTag3.Text("text", false);
				}
			}
		}

		CStr output = testFile.GetOutput();
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
		XMLWriter_File testFile;
		testFile.SetPrettyPrint(false);
		{
			XMLWriter_Element testTag1(testFile, "Test");
			{
				XMLWriter_Element testTag2(testFile, "example");
				{
					XMLWriter_Element testTag3(testFile, "content");
					testTag3.Text("text", false);
				}
			}
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<Test><example><content>text</content></example></Test>"
		);
	}

	void test_text()
	{
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Test");
			rootTag.Text("a", false);
			rootTag.Text("b", false);
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"\n"
			"<Test>ab</Test>"
		);
	}


	void test_utf8()
	{
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Test");
			{
				const wchar_t text[] = { 0x0251, 0 };
				rootTag.Text(text, false);
			}
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test>\xC9\x91</Test>"
		);
	}

	void test_attr_escape()
	{
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Test");
			rootTag.Attribute("example", "abc > ]]> < & \"\" ");
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test example=\"abc > ]]> &lt; &amp; &quot;&quot; \"/>"
		);
	}

	void test_chardata_escape()
	{
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Test");
			rootTag.Text("abc > ]]> < & \"\" ", false);
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test>abc > ]]&gt; &lt; &amp; \"\" </Test>"
		);
	}

	void test_cdata_escape()
	{
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Test");
			rootTag.Text("abc > ]]> < & \"\" ", true);
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test><![CDATA[abc > ]]>]]&gt;<![CDATA[ < & \"\" ]]></Test>"
		);
	}

	void test_comment_escape()
	{
		XMLWriter_File testFile;
		{
			XMLWriter_Element rootTag(testFile, "Test");
			testFile.Comment("test - -- --- ---- test");
		}

		CStr output = testFile.GetOutput();
		TS_ASSERT_STR_EQUALS(output,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n"
			"<Test>\n"
			"\t<!-- test - \xE2\x80\x90\xE2\x80\x90 \xE2\x80\x90\xE2\x80\x90- \xE2\x80\x90\xE2\x80\x90\xE2\x80\x90\xE2\x80\x90 test -->\n"
			"</Test>"
		);
	}
};
