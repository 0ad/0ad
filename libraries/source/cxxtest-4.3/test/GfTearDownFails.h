//
// This file tests what happens when GlobalFixture::tearDown() fails
//

#include <cxxtest/TestSuite.h>
#include <cxxtest/GlobalFixture.h>
#include <stdio.h>

class Fixture : public CxxTest::GlobalFixture
{
public:
    bool tearDown() { return false; }
};

//
// We can rely on this file being included exactly once
// and declare this global variable in the header file.
//
static Fixture fixture;

class Suite : public CxxTest::TestSuite
{
public:
    void testOne() {}
    void testTwo() { TS_WARN("Testing should go on!"); }
};
