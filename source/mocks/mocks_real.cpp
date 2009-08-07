#define CXXTEST_MOCK_REAL_SOURCE_FILE

// Pull in the common config headers from precompiled.h,
// but disable the actual precompiling (since we've only got
// one source file)
#ifdef USING_PCH
# undef USING_PCH
#endif
#include "lib/precompiled.h"

#include "mocks/boost_filesystem.h"

#if OS_LINUX
#include "mocks/dlfcn.h"
#endif // OS_LINUX
