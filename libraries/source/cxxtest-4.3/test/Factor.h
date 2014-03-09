//
// This file is used to test WorldDescription::strTotalTests()
//

#include <cxxtest/TestSuite.h>
#include <cxxtest/DummyDescriptions.h>

class Factor : public CxxTest::TestSuite
{
public:
    class X : public CxxTest::DummyWorldDescription
    {
    public:
        unsigned n;
        unsigned numTotalTests() const { return n; }
    };

    X x;
    enum Limit { MAX_STRLEN_TOTAL_TESTS = CxxTest::WorldDescription::MAX_STRLEN_TOTAL_TESTS };
    char buffer[MAX_STRLEN_TOTAL_TESTS * 2];

    const char *convert(unsigned n)
    {
        x.n = n;
        return x.strTotalTests(buffer);
    }

    void test_Some_numbers()
    {
        TS_WARN(convert(53));
        for (unsigned n = 0; n < 64; ++ n)
        {
            TS_ASSERT_DIFFERS(n, 32);
            TS_WARN(convert(n));
        }
    }

    class ShorterThan
    {
    public:
        bool operator()(const char *s, unsigned n) const
        {
            unsigned len = 0;
            while (*s++ != '\0')
            {
                ++ len;
            }
            return (len < n);
        }
    };

    class NotShorterThan
    {
        ShorterThan _shorterThan;

    public:
        bool operator()(const char *s, unsigned n) const { return !_shorterThan(s, n); }
    };

    void test_Lengths()
    {
        unsigned reasonableLimit = 60060;
        for (unsigned n = 0; n < reasonableLimit; ++ n)
        {
            TS_ASSERT_RELATION(ShorterThan, convert(n), MAX_STRLEN_TOTAL_TESTS);
        }
        TS_ASSERT_RELATION(NotShorterThan, convert(reasonableLimit), MAX_STRLEN_TOTAL_TESTS);
    }
};
