#include <cxxtest/TestSuite.h>

class EmptySuite : public CxxTest::TestSuite
{
public:
    static EmptySuite *createSuite() { return new EmptySuite(); }
    static void destroySuite(EmptySuite *suite) { delete suite; }

    void setUp() {}
    void tearDown() {}

    void thisSuiteHasNoTests()
    {
        TS_FAIL("This suite has no tests");
    }
};
