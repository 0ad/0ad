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
		logger->Log(CLogger::Normal, L"", L"Test 1");
		logger->Log(CLogger::Normal, L"", L"Test 2");

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 2);
		TS_ASSERT_EQUALS(lines[0], L"Test 1");
		TS_ASSERT_EQUALS(lines[1], L"Test 2");
	}

	void test_overflow()
	{
		const int buflen = 512;

		std::wstring msg0 (buflen-2, '*');
		std::wstring msg1 (buflen-1, '*');
		std::wstring msg2 (buflen,   '*');
		std::wstring msg3 (buflen+1, '*');

		std::wstring clipped (buflen-4, '*');
		clipped += L"...";

		logger->Log(CLogger::Normal, L"", L"%ls", msg0.c_str());
		logger->Log(CLogger::Normal, L"", L"%ls", msg1.c_str());
		logger->Log(CLogger::Normal, L"", L"%ls", msg2.c_str());
		logger->Log(CLogger::Normal, L"", L"%ls", msg3.c_str());

		logger->LogOnce(CLogger::Normal, L"", L"%ls", msg0.c_str());
		logger->LogOnce(CLogger::Normal, L"", L"%ls", msg1.c_str());
		logger->LogOnce(CLogger::Normal, L"", L"%ls", msg2.c_str());
		logger->LogOnce(CLogger::Normal, L"", L"%ls", msg3.c_str());

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 7);
		TS_ASSERT_EQUALS(lines[0], msg0);
		TS_ASSERT_EQUALS(lines[1], msg1);
		TS_ASSERT_EQUALS(lines[2], clipped);
		TS_ASSERT_EQUALS(lines[3], clipped);
		TS_ASSERT_EQUALS(lines[4], msg0);
		TS_ASSERT_EQUALS(lines[5], msg1);
		TS_ASSERT_EQUALS(lines[6], clipped);
	}

	//////////////////////////////////////////////////////////////////////////

	CLogger* logger;
	std::wstringstream* mainlog;
	std::wstringstream* interestinglog;
	std::vector<std::wstring> lines;

	void setUp()
	{
		mainlog = new std::wstringstream();
		interestinglog = new std::wstringstream();

		logger = new CLogger(mainlog, interestinglog, true, true);

		lines.clear();
	}

	void tearDown()
	{
		delete logger;
		logger = NULL;
	}

	void ParseOutput()
	{
		const std::wstring header_end = L"</h2>\n";

		std::wstring s = mainlog->str();
		size_t start = s.find(header_end);
		TS_ASSERT_DIFFERS(start, s.npos);
		s = s.substr(start + header_end.length());

		size_t n = 0, m;
		while (s.npos != (m = s.find('\n', n)))
		{
			lines.push_back(s.substr(n+3, m-n-7)); // strip the <p> and </p>
			n = m+1;
		}
	}
};
