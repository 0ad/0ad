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
		logger->LogMessage(L"Test 1");
		logger->LogMessage(L"Test 2");

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 2);
		TS_ASSERT_EQUALS(lines[0], "Test 1");
		TS_ASSERT_EQUALS(lines[1], "Test 2");
	}

	void test_overflow()
	{
		const int buflen = 1024;

		std::string msg0 (buflen-2, '*');
		std::string msg1 (buflen-1, '*');
		std::string msg2 (buflen,   '*');
		std::string msg3 (buflen+1, '*');

		std::string clipped (buflen-4, '*');
		clipped += "...";

		logger->LogMessage(L"%hs", msg0.c_str());
		logger->LogMessage(L"%hs", msg1.c_str());
		logger->LogMessage(L"%hs", msg2.c_str());
		logger->LogMessage(L"%hs", msg3.c_str());

		logger->LogMessageRender(L"%hs", msg0.c_str());
		logger->LogMessageRender(L"%hs", msg1.c_str());
		logger->LogMessageRender(L"%hs", msg2.c_str());
		logger->LogMessageRender(L"%hs", msg3.c_str());

		logger->LogWarning(L"%hs", msg0.c_str());
		logger->LogWarning(L"%hs", msg1.c_str());
		logger->LogWarning(L"%hs", msg2.c_str());
		logger->LogWarning(L"%hs", msg3.c_str());

		logger->LogError(L"%hs", msg0.c_str());
		logger->LogError(L"%hs", msg1.c_str());
		logger->LogError(L"%hs", msg2.c_str());
		logger->LogError(L"%hs", msg3.c_str());

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 4*4);
		TS_ASSERT_EQUALS(lines[0], msg0);
		TS_ASSERT_EQUALS(lines[1], msg1);
		TS_ASSERT_EQUALS(lines[2], clipped);
		TS_ASSERT_EQUALS(lines[3], clipped);

		TS_ASSERT_EQUALS(lines[4], msg0);
		TS_ASSERT_EQUALS(lines[5], msg1);
		TS_ASSERT_EQUALS(lines[6], clipped);
		TS_ASSERT_EQUALS(lines[7], clipped);

		TS_ASSERT_EQUALS(lines[8], "WARNING: "+msg0);
		TS_ASSERT_EQUALS(lines[9], "WARNING: "+msg1);
		TS_ASSERT_EQUALS(lines[10], "WARNING: "+clipped);
		TS_ASSERT_EQUALS(lines[11], "WARNING: "+clipped);

		TS_ASSERT_EQUALS(lines[12], "ERROR: "+msg0);
		TS_ASSERT_EQUALS(lines[13], "ERROR: "+msg1);
		TS_ASSERT_EQUALS(lines[14], "ERROR: "+clipped);
		TS_ASSERT_EQUALS(lines[15], "ERROR: "+clipped);
	}

	void test_unicode()
	{
		logger->LogMessage(L"%lc %lc", 226, 295);

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 1);
		TS_ASSERT_EQUALS(lines[0], "\xC3\xA2 \xC4\xA7");
	}

	void test_html()
	{
		logger->LogMessage(L"Test<a&b>c<d&e>");

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
