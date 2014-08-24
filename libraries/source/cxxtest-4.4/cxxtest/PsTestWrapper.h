#ifndef __cxxtest__PsTestWrapper_h__
#define __cxxtest__PsTestWrapper_h__

#include <cxxtest/TestMain.h>
#include <cxxtest/Gui.h>
// (This is not actually a GUI, but we use the CxxTest GUI mechanism
// to capture argv and do some special processing)

// We want this to act like Win32Gui on Windows, but like the default
// empty undisplayed GUI elsewhere
#ifdef _WIN32
# define PS_TEST_WRAPPER_BASE Win32Gui
# include <cxxtest/Win32Gui.h>
#else
# define PS_TEST_WRAPPER_BASE GuiListener
#endif

#include "ps/DllLoader.h"

namespace CxxTest
{
extern const char *g_PsOnlySuite;
extern const char *g_PsOnlyTest;

class PsTestRunner : public TestRunner
{
public:
    static void runAllTests(TestListener &listener)
    {
        // Code copied from TestRunner:
        tracker().setListener(&listener);
        _TS_TRY { PsTestRunner().runWorld(); }
        _TS_LAST_CATCH({ tracker().failedTest(__FILE__, __LINE__, "Exception thrown from world"); });
        tracker().setListener(0);
    }

    void runWorld()
    {
        RealWorldDescription wd;
        WorldGuard sg;

        // Allow all but one suite/test to be disabled
        if (g_PsOnlySuite)
        {
            for (SuiteDescription *sd = wd.firstSuite(); sd; sd = sd->next())
            {
                if (stringsEqual(sd->suiteName(), g_PsOnlySuite))
                {
                    for (TestDescription *td = sd->firstTest(); td; td = td->next())
                        if (g_PsOnlyTest && !stringsEqual(td->testName(), g_PsOnlyTest))
                        {
                            td->setActive(false);
                        }
                }
                else
                {
                    sd->setActive(false);
                }
            }
        }
        else
        {
            // By default, disable all tests with names containing "DISABLED"
            // (they can only be run by explicitly naming the suite/test)
            for (SuiteDescription *sd = wd.firstSuite(); sd; sd = sd->next())
            {
                for (TestDescription *td = sd->firstTest(); td; td = td->next())
                    if (strstr(td->testName(), "DISABLED"))
                    {
                        td->setActive(false);
                    }
            }
        }

        // Code copied from TestRunner:
        tracker().enterWorld(wd);
        if (wd.setUp())
        {
            for (SuiteDescription *sd = wd.firstSuite(); sd; sd = sd->next())
                if (sd->active())
                {
                    runSuite(*sd);
                }
            wd.tearDown();
        }
        tracker().leaveWorld(wd);
    }
};

class PsTestWrapper : public PS_TEST_WRAPPER_BASE
{
public:
    virtual void runGui(TestListener &listener)
    {
        enterGui();
        PsTestRunner::runAllTests(listener);
        leaveGui();
    }
    virtual bool process_commandline_args(int& i, int& argc, char* argv[])
    {
        if (i == 0)
        {
            return true;
        }

        if (!strcmp(argv[i], "-libdir"))
        {
            if (++i >= argc) // TODO: warn about missing arg
            {
                return false;
            }

            DllLoader::OverrideLibdir(argv[i]);
        }
        else if (!strcmp(argv[i], "-test"))
        {
            // NOTE: This does have slightly different behaviour than just specifying
            //       <suitename> <testname> on the command line if <testname> contains
            //       DISABLED.
            if (++i >= argc)
            {
                return false;
            }

            char *test = argv[i];
            // Try splitting as "suite::test", else use the whole string as the suite name
            char *colons = strstr(test, "::");
            if (colons)
            {
                colons[0] = '\0'; // (modifying argv is safe enough)
                g_PsOnlySuite = test;
                g_PsOnlyTest = colons + 2;
            }
            else
            {
                g_PsOnlySuite = test;
            }
        }
        else
        {
            return false;
        }

        return true;
    }
    virtual void print_help(const char* name)
    {
        CXXTEST_STD(cerr) << name << " -test TestSuiteName\n";
        CXXTEST_STD(cerr) << name << " -test TestSuiteName::test_case_name\n";
        CXXTEST_STD(cerr) << name << " -libdir <dir>" << CXXTEST_STD(endl);
    }
};
}

#endif // __cxxtest__PsTestWrapper_h__
