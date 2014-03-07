// -*- C++ -*-

#define CXXTEST_ABORT_TEST_ON_FAIL

// CxxTest definitions and headers
<CxxTest preamble>

// Make sure this worked
#ifndef TS_ASSERT
#   error The preamble does not work!
#endif

#include <cxxtest/StdioPrinter.h>

int main()
{
    CxxTest::StdioPrinter runner;

    TS_FAIL( "This will not be displayed" );
    int result = runner.run() + runner.run();
    TS_FAIL( "This will not be displayed" );
    
    return result;
}


// The CxxTest "world"
<CxxTest world>

