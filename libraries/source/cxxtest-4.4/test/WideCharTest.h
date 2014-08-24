#define CXXTEST_HAVE_STD
#include <cxxtest/TestSuite.h>
#include <string>

//
// This test suite tests CxxTest's conversion of wchar_t-related values to strings
//

class WideCharTest : public CxxTest::TestSuite
{
public:
    void testWideStringTraits()
    {
        TS_FAIL(std::basic_string<wchar_t>(L"std::wstring is displayed with L\"\""));
        wchar_t array[] = { (wchar_t)0x1234, (wchar_t)0x5678 };
        TS_FAIL(std::basic_string<wchar_t>(array, 2));
    }
};
