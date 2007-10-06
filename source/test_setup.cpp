// Got to be consistent with what the rest of the source files do before
// including precompiled.h, so that the PCH works correctly
#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif
#define _CXXTEST_HAVE_STD

#include "precompiled.h"

#include <cxxtest/GlobalFixture.h>

class LeakReporter : public CxxTest::GlobalFixture
{
	virtual bool tearDownWorld()
	{
		// Enable leak reporting on exit.
		// (This is done in tearDownWorld so that it doesn't report 'leaks'
		// if the program is aborted before finishing cleanly.)

#ifdef HAVE_VC_DEBUG_ALLOC
		int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		flags |= _CRTDBG_LEAK_CHECK_DF; // check for memory leaks
		flags |= _CRTDBG_ALLOC_MEM_DF; // also check allocs using the non-debug version of new
		_CrtSetDbgFlag(flags);

		// Send output to stdout as well as the debug window, so it works during
		// the normal build process as well as when debugging the test .exe
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif

		return true;
	}

	virtual bool setUpWorld()
	{
#ifdef _MSC_VER
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
