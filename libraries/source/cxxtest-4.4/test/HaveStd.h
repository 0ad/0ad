#include <cxxtest/TestSuite.h>

//
// This tests CxxTest's `--have-std' option
//
#include "Something.h"

class HaveStd : public CxxTest::TestSuite
{
public:
    void testHaveStd()
    {
        TS_ASSERT_EQUALS(something(), "Something");
    }
};
