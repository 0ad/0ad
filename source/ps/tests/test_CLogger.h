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

#include "ps/CLogger.h"

class TestCLogger : public CxxTest::TestSuite
{
public:
	void test_basic()
	{
		logger->WriteMessage("Test 1", false);
		logger->WriteMessage("Test 2", false);

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 2);
		TS_ASSERT_EQUALS(lines[0], "Test 1");
		TS_ASSERT_EQUALS(lines[1], "Test 2");
	}

	void test_unicode()
	{
		wchar_t str[] = { 226, 32, 295, 0 };
		logger->WriteMessage(utf8_from_wstring(str).c_str(), false);

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 1);
		TS_ASSERT_EQUALS(lines[0], "\xC3\xA2 \xC4\xA7");
	}

	void test_html()
	{
		logger->WriteMessage("Test<a&b>c<d&e>", false);

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 1);
		TS_ASSERT_EQUALS(lines[0], "Test&lt;a&amp;b>c&lt;d&amp;e>");
	}

	//////////////////////////////////////////////////////////////////////////

	CLogger* logger;
	std::stringstream* mainlog;
	std::stringstream* interestinglog;
	std::vector<std::string> lines;

	void setUp()
	{
		mainlog = new std::stringstream();
		interestinglog = new std::stringstream();

		logger = new CLogger(mainlog, interestinglog, true, false);

		lines.clear();
	}

	void tearDown()
	{
		delete logger;
		logger = NULL;
	}

	void ParseOutput()
	{
		const std::string header_end = "</h2>\n";

		std::string s = mainlog->str();
		size_t start = s.find(header_end);
		TS_ASSERT_DIFFERS(start, s.npos);
		s = s.substr(start + header_end.length());

		size_t n = 0, m;
		while (s.npos != (m = s.find('\n', n)))
		{
			size_t gt = s.find('>', n);
			lines.push_back(s.substr(gt+1, m-gt-5)); // strip the <p> and </p>
			n = m+1;
		}
	}
};
