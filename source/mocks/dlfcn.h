#include <dlfcn.h>
#include <cxxtest/Mock.h>
CXXTEST_MOCK_GLOBAL(
	int, dladdr,
	(void *addr, Dl_info *info),
	(addr, info)
);
