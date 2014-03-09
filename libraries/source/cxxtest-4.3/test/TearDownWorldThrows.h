//
// This file tests what happens when GlobalFixture::tearDownWorld() throws
//

#include <cxxtest/TestSuite.h>
#include <cxxtest/GlobalFixture.h>
#include <stdio.h>

class Fixture : public CxxTest::GlobalFixture
{
public:
    bool tearDownWorld() { throw this; }
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
};
