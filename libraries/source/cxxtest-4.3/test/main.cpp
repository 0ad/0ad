#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <stdio.h>

//
// This test runner printer some statistics at the end of the run.
// Note that it uses <stdio.h> and not <iostream> for compatibility
// with older compilers.
//

using namespace CxxTest;

class SummaryPrinter : public CxxTest::TestListener
{
public:
    void run()
    {
        CxxTest::TestRunner::runAllTests(*this);
    }

    void leaveWorld(const CxxTest::WorldDescription &wd)
    {
        printf("Number of suites: %u\n", wd.numSuites());
        printf("Number of tests: %u\n", wd.numTotalTests());
        printf("Number of failed tests: %u\n", TestTracker::tracker().failedTests());
        printf("Number of skipped tests: %u\n", TestTracker::tracker().skippedTests());
    }
};

int main()
{
    SummaryPrinter().run();
    return 0;
}
