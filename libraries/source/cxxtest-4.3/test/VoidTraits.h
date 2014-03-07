//
// This include file is used to test the --include option
//

#ifdef CXXTEST_RUNNING

#include <cxxtest/ValueTraits.h>

namespace CxxTest
{
CXXTEST_TEMPLATE_INSTANTIATION
class ValueTraits<void *>
{
public:
    ValueTraits(void *) {}
    const char *asString(void) { return "(void *)"; }
};
}

#endif
