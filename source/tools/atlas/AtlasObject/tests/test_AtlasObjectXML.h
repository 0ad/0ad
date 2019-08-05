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

#include "../AtlasObject.h"

class TestAtlasObjectXML : public CxxTest::TestSuite
{
private:
	void try_parse_save(const char* in, const char* out)
	{
		AtObj obj = AtlasObject::LoadFromXML(in);
		std::string xml = AtlasObject::SaveToXML(obj);
		TS_ASSERT_EQUALS(xml, out);
	}

	void try_parse_error(const char* in)
	{
		AtObj obj = AtlasObject::LoadFromXML(in);
		TS_ASSERT(!obj.defined());
	}

public:
	void test_parse_basic()
	{
		try_parse_save(
			"<foo/>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo/>\n"
		);
	}
	void test_parse_child()
	{
		try_parse_save(
			"<foo><bar/></foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>\n"
			"  <bar/>\n"
			"</foo>\n"
		);
	}
	void test_parse_attributes1()
	{
		try_parse_save(
			"<foo><bar baz='qux'/></foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>\n"
			"  <bar baz=\"qux\"/>\n"
			"</foo>\n"
		);
	}
	void test_parse_attributes2()
	{
		try_parse_save(
			"<foo a='b'><bar baz='qux' baz2='qux2'/></foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo a=\"b\">\n"
			"  <bar baz=\"qux\" baz2=\"qux2\"/>\n"
			"</foo>\n"
		);
	}
	void test_parse_text1()
	{
		try_parse_save(
			"<foo>bar</foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>bar</foo>\n"
		);
	}
	void test_parse_text2()
	{
		try_parse_save(
			"<foo><bar> baz </bar></foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>\n"
			"  <bar>baz</bar>\n"
			"</foo>\n"
		);
	}
	void test_parse_text3()
	{
		try_parse_save(
			"<foo><bar a='b'><x/>\t\n <y/>\n\t baz\n<z/></bar></foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>\n"
			"  <bar a=\"b\">baz<x/><y/><z/></bar>\n"
			"</foo>\n"
		);
	}
	void test_parse_text4()
	{
		try_parse_save(
			"<foo>a<![CDATA[bcd]]>e&amp;f&lt;g&gt;h&quot;i&apos;j</foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>abcde&amp;f&lt;g&gt;h\"i'j</foo>\n"
		);
	}
	void test_parse_doctype()
	{
		try_parse_save(
			"<!DOCTYPE foo SYSTEM \"file:///dev/urandom\"><foo/>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo/>\n"
		);
	}
	void test_parse_ignored()
	{
		try_parse_save(
			"<!DOCTYPE foo><foo> a <!-- xxx --> b <?xxx?></foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>a  b</foo>\n"
		);
	}

	void test_parse_unicode()
	{
		try_parse_save(
			"<?xml version=\"1.0\" encoding=\"utf-8\"?><foo x='&#x1234;\xE1\x88\xB4'>&#x1234;\xE1\x88\xB4</foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo x=\"\xE1\x88\xB4\xE1\x88\xB4\">\xE1\x88\xB4\xE1\x88\xB4</foo>\n"
		);
	}

	void test_parse_unicode_nonbmp()
	{
		try_parse_save(
			"<?xml version=\"1.0\" encoding=\"utf-8\"?><foo>&#xfffc;&#xfffd;&#x10000;&#x10ffff;</foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo>\xEF\xBF\xBC\xEF\xBF\xBD\xF0\x90\x80\x80\xF4\x8F\xBF\xBF</foo>\n"
		);
	}

	void test_parse_iso88591()
	{
		try_parse_save(
			"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?><foo x='&#x1234;\xE1\x88\xB4'>&#x1234;\xE1\x88\xB4</foo>",
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<foo x=\"\xE1\x88\xB4\xC3\xA1\xC2\x88\xC2\xB4\">\xE1\x88\xB4\xC3\xA1\xC2\x88\xC2\xB4</foo>\n"
		);
	}

	void XXX_test_parse_bogus_entity() // TODO: enable this once errors aren't just dumped to stdout
	{
		try_parse_error(
			"<foo>&cheese;</foo>"
		);
	}
};
