/*
-------------------------------------------------------------------------
 CxxTest: A lightweight C++ unit testing library.
 Copyright (c) 2008 Sandia Corporation.
 This software is distributed under the LGPL License v3
 For more information, see the COPYING file in the top CxxTest directory.
 Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------
*/

#ifndef __CxxTestMain_h
#define __CxxTestMain_h

#include <cxxtest/TestTracker.h>
#include <cxxtest/Flags.h>

#ifndef _CXXTEST_HAVE_STD
#   define _CXXTEST_HAVE_STD
#endif // _CXXTEST_HAVE_STD

#include <cxxtest/StdValueTraits.h>

#if defined(_CXXTEST_HAVE_STD)
#ifdef _CXXTEST_OLD_STD
#   include <iostream.h>
#   include <string.h>
#else // !_CXXTEST_OLD_STD
#   include <iostream>
#   include <cstring>
#endif // _CXXTEST_OLD_STD

namespace CxxTest
{

template <class TesterT>
inline void print_help(TesterT& tmp, const char* name)
{
    CXXTEST_STD(cerr) << name << " <suitename>\n";
    CXXTEST_STD(cerr) << name << " <suitename> <testname>\n";
    CXXTEST_STD(cerr) << name << " -h\n";
    CXXTEST_STD(cerr) << name << " --help\n";
    CXXTEST_STD(cerr) << name << " --help-tests\n";
    CXXTEST_STD(cerr) << name << " -v             Enable tracing output.\n";
    CXXTEST_STD(cerr) << "Frontend specific options:" << CXXTEST_STD(endl);
    tmp.print_help(name);
}
#endif


template <class TesterT>
int Main(TesterT& tmp, int argc, char* argv[])
{
//
// Parse the command-line arguments. The default behavior is to run all tests
//
// This is a primitive parser, but I'm not sure what sort of portable
// parser should be used in cxxtest.
//
    int i = 0;
    tmp.process_commandline_args(i, argc, argv);

#if defined(_CXXTEST_HAVE_STD)
    bool suiteSpecified = false;
    for (i = 1; i < argc; i++)
    {
        if ((CXXTEST_STD(strcmp)(argv[i], "-h") == 0) || (CXXTEST_STD(strcmp)(argv[i], "--help") == 0))
        {
            print_help(tmp, argv[0]);
            return 0;
        }
        else if ((CXXTEST_STD(strcmp)(argv[i], "--help-tests") == 0))
        {
            CXXTEST_STD(cout) << "Suite/Test Names" << CXXTEST_STD(endl);
            CXXTEST_STD(cout) << "---------------------------------------------------------------------------" << CXXTEST_STD(endl);
            for (SuiteDescription *sd = RealWorldDescription().firstSuite(); sd; sd = sd->next())
                for (TestDescription *td = sd->firstTest(); td; td = td->next())
                {
                    CXXTEST_STD(cout) << td->suiteName() << " " << td->testName() << CXXTEST_STD(endl);
                }
            return 0;
        }
        else if (CXXTEST_STD(strcmp)(argv[i], "-v") == 0)
        {
            tracker().print_tracing = true;
        }
        else if (argv[i][0] == '-')
        {
            // Increments i if it handles more arguments
            if (!tmp.process_commandline_args(i, argc, argv))
            {
                CXXTEST_STD(cerr) << "ERROR: unknown option '" << argv[i] << "'" << CXXTEST_STD(endl);
                return -1;
            }
        }
        else
        {
            if (suiteSpecified)
            {
                CXXTEST_STD(cerr) << "ERROR: only one suite or one test can be specified" << CXXTEST_STD(endl);
                return -1;
            }
            suiteSpecified = true;
            bool status = false;
            // Test case and test suite
            if (i + 1 < argc) // suite and test
            {
                status = leaveOnly(argv[i], argv[i + 1]);
                if (!status)
                {
                    CXXTEST_STD(cerr) << "ERROR: unknown test '" << argv[i] << "::" << argv[i + 1] << "'" << CXXTEST_STD(endl);
                    return -1;
                }
                i++; // We handled two parameters
            }
            else // suite
            {
                // TODO also handle "Suite::Test"?
                status = leaveOnly(argv[i]);
                if (!status)
                {
                    CXXTEST_STD(cerr) << "ERROR: unknown suite '" << argv[i] << "'" << CXXTEST_STD(endl);
                    return -1;
                }
            }
        }
    }
#endif

    return tmp.run();
}

}
#endif

