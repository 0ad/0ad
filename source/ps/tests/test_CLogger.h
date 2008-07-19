#include "lib/self_test.h"

#include "ps/CLogger.h"

class TestCLogger : public CxxTest::TestSuite 
{
public:
	void test_basic()
	{
		logger->Log(CLogger::Normal, "", "Test 1");
		logger->Log(CLogger::Normal, "", "Test 2");

		ParseOutput();

		TS_ASSERT_EQUALS((int)lines.size(), 2);
		TS_ASSERT_EQUALS(lines[0], "Test 1");
		TS_ASSERT_EQUALS(lines[1], "Test 2");
	}

	void test_overflow()
	{
		const int buflen = 512;

		std::string msg0 (buflen-2, '*');
		std::string msg1 (buflen-1, '*');
		std::string msg2 (buflen,   '*');
		std::string msg3 (buflen+1, '*');

		std::string clipped (buflen-4, '*');
		clipped += "...";

		logger->Log(CLogger::Normal, "", msg0.c_str());
		logger->Log(CLogger::Normal, "", msg1.c_str());
		logger->Log(CLogger::Normal, "", msg2.c_str());
		logger->Log(CLogger::Normal, "", msg3.c_str());

		logger->LogOnce(CLogger::Normal, "", msg0.c_str());
		logger->LogOnce(CLogger::Normal, "", msg1.c_str());
		logger->LogOnce(CLogger::Normal, "", msg2.c_str());
		logger->LogOnce(CLogger::Normal, "", msg3.c_str());

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
	std::stringstream* mainlog;
	std::stringstream* interestinglog;
	std::vector<std::string> lines;

	void setUp()
	{
		mainlog = new std::stringstream();
		interestinglog = new std::stringstream();

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
		const std::string header_end = "</h1>\n";

		std::string s = mainlog->str();
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
