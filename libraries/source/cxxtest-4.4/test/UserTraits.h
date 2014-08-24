//
// This sample demonstrates rolling your own ValueTraits.
// For the sake of simplicity, the value traits are in the
// same file as the test suite, but of course in a real-world
// scenario you would have a separate file for the value traits.
//
// This file should be used with the template file UserTraits.tpl
//


//
// Declare our own ValueTraits<int> which converts to hex notation
//
#include <cxxtest/ValueTraits.h>
#include <stdio.h>

namespace CxxTest
{
CXXTEST_TEMPLATE_INSTANTIATION
class ValueTraits<int>
{
    char _asString[128]; // Crude, but it should be enough
public:
    ValueTraits(int i) { sprintf(_asString, "0x%X", i); }
    const char *asString(void) { return _asString; }
};
}

class TestUserTraits : public CxxTest::TestSuite
{
public:
    void testUserTraits()
    {
        TS_FAIL(127);
    }
};
