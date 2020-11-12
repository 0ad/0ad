/* Copyright (C) 2020 Wildfire Games.
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

#include <fstream>

#include "lib/self_test.h"
#include <cxxtest/GlobalFixture.h>

#if OS_WIN
#include "lib/sysdep/os/win/wdbg_heap.h"
#endif

#include "lib/timer.h"
#include "lib/sysdep/sysdep.h"
#include "ps/Profiler2.h"
#include "scriptinterface/ScriptEngine.h"
#include "scriptinterface/ScriptRuntime.h"

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

class MiscSetup : public CxxTest::GlobalFixture
{
	virtual bool setUpWorld()
	{
		// Timer must be initialised, else things will break when tests do IO
		timer_LatchStartTime();

#if OS_MACOSX || OS_BSD
		// See comment in GameSetup.cpp FixLocales
		setlocale(LC_CTYPE, "UTF-8");
#endif

		ThreadUtil::SetMainThread();

		g_Profiler2.Initialise();
		m_ScriptEngine = new ScriptEngine;
		g_ScriptRuntime = ScriptRuntime::CreateRuntime();

		return true;
	}

	virtual bool tearDownWorld()
	{
		g_ScriptRuntime.reset();
		SAFE_DELETE(m_ScriptEngine);
		g_Profiler2.Shutdown();

		return true;
	}

private:

	// We're doing the initialization and shutdown of the ScriptEngine explicitly here
	// to make sure it's only initialized when setUpWorld is called.
	ScriptEngine* m_ScriptEngine;
};

static LeakReporter leakReporter;
static MiscSetup miscSetup;

// Definition of functions from lib/self_test.h

bool ts_str_contains(const std::string& str1, const std::string& str2)
{
	return str1.find(str2) != str1.npos;
}

bool ts_str_contains(const std::wstring& str1, const std::wstring& str2)
{
	return str1.find(str2) != str1.npos;
}

// we need the (version-controlled) binaries/data directory because it
// contains input files (it is assumed that developer's machines have
// write access to those directories). note that argv0 isn't
// available, so we use sys_ExecutablePathname.
OsPath DataDir()
{
	return sys_ExecutablePathname().Parent()/".."/"data";
}

// Script-based testing setup:

namespace
{
	void script_TS_FAIL(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& msg)
	{
		TS_FAIL(utf8_from_wstring(msg).c_str());
	}
}

void ScriptTestSetup(const ScriptInterface& scriptinterface)
{
	scriptinterface.RegisterFunction<void, std::wstring, script_TS_FAIL>("TS_FAIL");

	// Load the TS_* function definitions
	// (We don't use VFS because tests might not have the normal VFS paths loaded)
	OsPath path = DataDir()/"tests"/"test_setup.js";
	std::ifstream ifs(OsString(path).c_str());
	ENSURE(ifs.good());
	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	bool ok = scriptinterface.LoadScript(L"test_setup.js", content);
	ENSURE(ok);
}
