#define CXXTEST_MOCK_TEST_SOURCE_FILE
#include "lib/sysdep/os.h"

// Cause calls to be redirected to the real function by default
#define DEFAULT(name) static T::Real_##name real_##name

#include "mocks/boost_filesystem.h"
DEFAULT(Boost_Filesystem_initial_path);

#if OS_LINUX

#include "mocks/dlfcn.h"
DEFAULT(dladdr);

#endif // OS_LINUX
