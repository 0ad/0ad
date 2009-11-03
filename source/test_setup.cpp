/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
bool ts_str_contains(const std::wstring& str1, const std::wstring& str2)
{
	return str1.find(str2) != str1.npos;
}
