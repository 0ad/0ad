#define CXXTEST_HAVE_STD
#include <cxxtest/TestSuite.h>

//
// This test suite tests CxxTest's conversion of different values to strings
//

class TraitsTest : public CxxTest::TestSuite
{
public:
    void testIntegerTraits()
    {
        TS_FAIL((unsigned char)1);
        TS_FAIL((char)0x0F);
        TS_FAIL((signed short int) - 12);
        TS_FAIL((unsigned short int)34);
        TS_FAIL((signed int) - 123);
        TS_FAIL((unsigned int)456);
        TS_FAIL((signed long int) - 12345);
        TS_FAIL((unsigned long int)67890);
    }

    void testFloatingPointTraits()
    {
        TS_FAIL((float)0.12345678);
        TS_FAIL((double)0.12345678);
    }

    void testBoolTraits()
    {
        TS_FAIL(true);
        TS_FAIL(false);
    }

    void testCharTraits()
    {
        TS_FAIL('A');
        TS_FAIL('\x04');
        TS_FAIL('\x1B');
        TS_FAIL('\0');
        TS_FAIL('\r');
        TS_FAIL('\n');
        TS_FAIL('\b');
        TS_FAIL('\t');
        TS_FAIL('\a');
        TS_FAIL((char) - 5);
    }

    void testStringTraits()
    {
        TS_FAIL("(char *) is displayed as-is\n");
    }

    void testStdStringTraits()
    {
        typedef CXXTEST_STD(string) String;
        TS_FAIL(String("std::string is displayed with \"\""));
        TS_FAIL(String("Escapes\rAre\rTranslated"));
        TS_FAIL(String("As are unprintable chars: \x12\x34\x56\x78"));
    }
};
