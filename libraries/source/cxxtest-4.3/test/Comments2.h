#include <cxxtest/TestSuite.h>

//
// This is a test of commenting out tests in CxxTest
//

class Comments : public CxxTest::TestSuite
{
public:
    void test_Something()
    {
        TS_WARN("Something");
    }

    /*
        void test_Something_else()
        {
            TS_WARN( "Something else" );
        }
    */
};
