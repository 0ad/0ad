// Got to be consistent with what the rest of the source files do before
// including precompiled.h, so that the PCH works correctly
#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif
#define _CXXTEST_HAVE_STD

#include "precompiled.h"

#include <cxxtest/GlobalFixture.h>

#if OS_WIN
#include "lib/sysdep/os/win/wdbg_heap.h"
#endif

class LeakReporter : public CxxTest::GlobalFixture
{
	virtual bool tearDownWorld()
	{
		// Enable leak reporting on exit.
		// (This is done in tearDownWorld so that it doesn't report 'leaks'
		// if the program is aborted before finishing cleanly.)
#if OS_WIN
		wdbg_heap_Enable(true);
#endif
		return true;
	}

	virtual bool setUpWorld()
	{
#if MSC_VERSION
		// (Warning: the allocation numbers seem to differ by 3 when you
		// run in the build process vs the debugger)
		// _CrtSetBreakAlloc(1952);
#endif
		return true;
	}

};

static LeakReporter leakReporter;

// Definition of function from lib/self_test.h
bool ts_str_contains(const std::string& str1, const std::string& str2)
{
	return str1.find(str2) != str1.npos;
}
