#include "lib/self_test.h"

#include "ps/XML/Xeromyces.h"

#include <xercesc/framework/MemBufInputSource.hpp>

XERCES_CPP_NAMESPACE_USE

class TestXeroXMB : public CxxTest::TestSuite 
{
private:
	shared_ptr<u8> m_Buffer;

	XMBFile parse(const char* doc)
	{
		XMLPlatformUtils::Initialize();
		MemBufInputSource source((const XMLByte*)doc, strlen(doc), "null");
		WriteBuffer buffer;
		PSRETURN ret = CXeromyces::ConvertXMLtoXMB("/dev/null", source, buffer);
		TS_ASSERT_EQUALS(ret, PSRETURN_OK);

		XMBFile xmb;
		m_Buffer = buffer.Data(); // hold a reference
		TS_ASSERT(xmb.Initialise((const char*)m_Buffer.get()));
		return xmb;
	}

public:
	void test_basic()
	{
		XMBFile xmb (parse("<test>\n<foo x=' y '> bar </foo>\n</test>"));

		TS_ASSERT_DIFFERS(xmb.GetElementID("test"), -1);
		TS_ASSERT_DIFFERS(xmb.GetElementID("foo"), -1);
		TS_ASSERT_EQUALS(xmb.GetElementID("bar"), -1);

		TS_ASSERT_DIFFERS(xmb.GetAttributeID("x"), -1);
		TS_ASSERT_EQUALS(xmb.GetAttributeID("y"), -1);
		TS_ASSERT_EQUALS(xmb.GetAttributeID("test"), -1);

		int el_test = xmb.GetElementID("test");
		int el_foo = xmb.GetElementID("foo");
		int at_x = xmb.GetAttributeID("x");

		XMBElement root = xmb.GetRoot();
		TS_ASSERT_EQUALS(root.GetNodeName(), el_test);
//		TS_ASSERT_EQUALS(root.GetLineNumber(), 1);
		TS_ASSERT_EQUALS(CStr(root.GetText()), "");

		TS_ASSERT_EQUALS(root.GetChildNodes().Count, 1);
		XMBElement child = root.GetChildNodes().Item(0);
		TS_ASSERT_EQUALS(child.GetNodeName(), el_foo);
//		TS_ASSERT_EQUALS(child.GetLineNumber(), 2);
		TS_ASSERT_EQUALS(child.GetChildNodes().Count, 0);
		TS_ASSERT_EQUALS(CStr(child.GetText()), "bar");

		TS_ASSERT_EQUALS(child.GetAttributes().Count, 1);
		XMBAttribute attr = child.GetAttributes().Item(0);
		TS_ASSERT_EQUALS(attr.Name, at_x);
		TS_ASSERT_EQUALS(CStr(attr.Value), " y ");
	}

	void test_doctype_ignored()
	{
		XMBFile xmb (parse("<!DOCTYPE foo SYSTEM \"file:///dev/urandom\"><foo/>"));

		TS_ASSERT_DIFFERS(xmb.GetElementID("foo"), -1);
	}

	void test_complex_parse()
	{
		XMBFile xmb (parse("<test>\t\n \tx &lt;>&amp;&quot;&apos;<![CDATA[foo]]>bar\n<x/>\nbaz<?cheese?>qux</test>"));
		TS_ASSERT_EQUALS(CStr(xmb.GetRoot().GetText()), "x <>&\"'foobar\n\nbazqux");
	}

};
