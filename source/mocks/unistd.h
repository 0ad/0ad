#include <unistd.h>
#include <cxxtest/Mock.h>
CXXTEST_MOCK_GLOBAL(
	char *, getcwd,
	(char *buf, size_t size),
	(buf, size)
);
