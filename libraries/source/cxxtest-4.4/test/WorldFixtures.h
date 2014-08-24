//
// This file tests CxxTest global fixtures setUpWorld()/tearDownWorld()
//

#include <cxxtest/TestSuite.h>
#include <cxxtest/GlobalFixture.h>
#include <stdio.h>

class PrintingFixture : public CxxTest::GlobalFixture
{
public:
    bool setUpWorld() { printf("<world>"); return true; }
    bool tearDownWorld() { printf("</world>"); return true; }
    bool setUp() { printf("<test>"); return true; }
    bool tearDown() { printf("</test>"); return true; }
};

//
// We can rely on this file being included exactly once
// and declare this global variable in the header file.
//
static PrintingFixture printingFixture;

//
// Now define some tests
//

class FirstSuite : public CxxTest::TestSuite
{
public:
    void testOne() {}
    void testTwo() {}
};

class SecondSuite : public CxxTest::TestSuite
{
public:
    void testOne() {}
    void testTwo() {}
};
