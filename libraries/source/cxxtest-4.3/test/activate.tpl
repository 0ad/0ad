// -*- C++ -*-
# include  <cxxtest/StdioPrinter.h>
# include  <stdio.h>

int main()
{
    if ( !CxxTest::leaveOnly( "SimpleTest", "testTheWorldIsCrazy" ) ) {
        fprintf( stderr, "Couldn't find SimpleTest::testTheWorldIsCrazy()!?\n" );
        return -1;
    }

    CxxTest::activateAllTests();
    return CxxTest::StdioPrinter().run();
}


// The CxxTest "world"
<CxxTest world>

