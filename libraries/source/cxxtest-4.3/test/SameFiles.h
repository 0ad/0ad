#include <cxxtest/TestSuite.h>

//
// This test suite demonstrates TS_ASSERT_SAME_FILES
//

class SameFiles : public CxxTest::TestSuite
{
public:

    void testAssertFiles()
    {
        TS_ASSERT_SAME_FILES("SameFiles.h", "SameFiles.h");
    }

    void testAssertFileShorter()
    {
        TS_ASSERT_SAME_FILES("SameFiles.h", "SameFilesLonger.h");
    }

    void testAssertFileLonger()
    {
        TS_ASSERT_SAME_FILES("SameFilesLonger.h", "SameFiles.h");
    }

    void testAssertMessageSameFiles()
    {
        TSM_ASSERT_SAME_FILES("Not same files", "SameFiles.h", "SameData.h");
    }

    void testSafeAssertSameFiles()
    {
        ETS_ASSERT_SAME_FILES("SameFiles.h", "SameFiles.h");
    }

    void testSafeAssertMessageSameFiles()
    {
        ETSM_ASSERT_SAME_FILES("Not same files", "SameFiles.h", "SameData.h");
    }
};

