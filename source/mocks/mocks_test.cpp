#define CXXTEST_MOCK_TEST_SOURCE_FILE

// Pull in the common config headers from precompiled.h,
// but disable the actual precompiling (since we've only got
// one source file)
#ifdef USING_PCH
# undef USING_PCH
#endif
#include "lib/precompiled.h"

// Cause calls to be redirected to the real function by default
#define DEFAULT(name) static T::Real_##name real_##name

#if OS_LINUX

#include "mocks/dlfcn.h"
DEFAULT(dladdr);

#include "mocks/unistd.h"
DEFAULT(getcwd);

#endif // OS_LINUX
