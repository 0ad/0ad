#include "lib/external_libraries/boost_filesystem.h"
#include <cxxtest/Mock.h>
CXXTEST_MOCK(
	Boost_Filesystem_initial_path,
	boost::filesystem::path,
	Boost_Filesystem_initial_path,
	(),
	boost::filesystem::initial_path,
	()
);
