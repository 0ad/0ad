#define CXXTEST_MOCK_REAL_SOURCE_FILE
#include "lib/sysdep/os.h"

#include "mocks/boost_filesystem.h"

#if OS_LINUX
#include "mocks/dlfcn.h"
#endif // OS_LINUX
