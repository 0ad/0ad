#define CXXTEST_HAVE_EH
#include <cxxtest/ErrorPrinter.h>

int main( int argc, char *argv[] ) {
    CxxTest::ErrorPrinter tmp;
    return CxxTest::Main<CxxTest::ErrorPrinter>( tmp, argc, argv );
}

// The CxxTest "world"
<CxxTest world>
