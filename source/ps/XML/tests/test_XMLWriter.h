#include "lib/self_test.h"

#include "ps/XML/XML.h"
#include "ps/XML/XMLwriter.h"

class TestXmlWriter : public CxxTest::TestSuite 
{
public:
	void test1()
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
};