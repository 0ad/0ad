#include <cxxtest/Flags.h>

#ifndef _CXXTEST_PARTIAL_TEMPLATE_SPECIALIZATION
template<class T> class X {} x;
template<class T, class U> class Pair {} p;
template<class T, class U> class X< Pair<T, U> > {} xp;

int main() { return 0; }
#endif // !_CXXTEST_PARTIAL_TEMPLATE_SPECIALIZATION

