//
// This include file is used to test the --include option
//

#include <cxxtest/ValueTraits.h>

namespace CxxTest
{
CXXTEST_TEMPLATE_INSTANTIATION
class ValueTraits<long *>
{
public:
    ValueTraits(long *) {}
    const char *asString() { return "(long *)"; }
};
}
