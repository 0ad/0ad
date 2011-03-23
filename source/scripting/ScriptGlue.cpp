/* Copyright (C) 2010 Wildfire Games.
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

// This module defines the table of all functions callable from JS.
// it's required by the interpreter; we make use of the opportunity to
// document them all in one spot. we thus obviate having to dig through
// all the other headers. most of the functions are implemented here;
// as for the rest, we only link to their docs (duplication is bad).

#include "precompiled.h"

#include "ScriptGlue.h"
#include "JSConversions.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapWriter.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "lib/frequency_filter.h"
#include "lib/svn_revision.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/sysdep/sysdep.h"	// sys_OpenFile
#include "maths/scripting/JSInterface_Vector3D.h"
#include "network/NetServer.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/Globals.h"	// g_frequencyFilter
#include "ps/GameSetup/GameSetup.h"
#include "ps/Hotkey.h"
#include "ps/ProfileViewer.h"
#include "ps/World.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

// rationale: the function table is now at the end of the source file to
// avoid the need for forward declarations for every function.

// all normal function wrappers have the following signature:
//   JSBool func(JSContext* cx, JSObject* globalObject, uintN argc, jsval* argv, jsval* rval);
// all property accessors have the following signature:
//   JSBool accessor(JSContext* cx, JSObject* globalObject, jsval id, jsval* vp);


//-----------------------------------------------------------------------------
// Timer
//-----------------------------------------------------------------------------


// Script profiling functions: Begin timing a piece of code with StartJsTimer(num)
// and stop timing with StopJsTimer(num). The results will be printed to stdout
// when the game exits.

static const size_t MAX_JS_TIMERS = 20;
static TimerUnit js_start_times[MAX_JS_TIMERS];
static TimerUnit js_timer_overhead;
static TimerClient js_timer_clients[MAX_JS_TIMERS];
static wchar_t js_timer_descriptions_buf[MAX_JS_TIMERS * 12];	// depends on MAX_JS_TIMERS and format string below

static void InitJsTimers()
{
	wchar_t* pos = js_timer_descriptions_buf;
	for(size_t i = 0; i < MAX_JS_TIMERS; i++)
	{
		const wchar_t* description = pos;
		pos += swprintf_s(pos, 12, L"js_timer %d", (int)i)+1;
		timer_AddClient(&js_timer_clients[i], description);
	}

	// call several times to get a good approximation of 'hot' performance.
	// note: don't use a separate timer slot to warm up and then judge
	// overhead from another: that causes worse results (probably some
	// caching effects inside JS, but I don't entirely understand why).
	static const char* calibration_script =
		"startXTimer(0);\n"
		"stopXTimer (0);\n"
		"\n";
	g_ScriptingHost.RunMemScript(calibration_script, strlen(calibration_script));
	// slight hack: call RunMemScript twice because we can't average several
	// TimerUnit values because there's no operator/. this way is better anyway
	// because it hopefully avoids the one-time JS init overhead.
	g_ScriptingHost.RunMemScript(calibration_script, strlen(calibration_script));
	js_timer_overhead = js_timer_clients[0].sum;
	js_timer_clients[0].sum.SetToZero();
}

JSBool StartJsTimer(JSContext* cx, uintN argc, jsval* vp)
{
	ONCE(InitJsTimers());

	JSU_REQUIRE_PARAMS(1);
	size_t slot = ToPrimitive<size_t>(JS_ARGV(cx, vp)[0]);
	if (slot >= MAX_JS_TIMERS)
		return JS_FALSE;

	js_start_times[slot].SetFromTimer();

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}


JSBool StopJsTimer(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_PARAMS(1);
	size_t slot = ToPrimitive<size_t>(JS_ARGV(cx, vp)[0]);
	if (slot >= MAX_JS_TIMERS)
		return JS_FALSE;

	TimerUnit now;
	now.SetFromTimer();
	now.Subtract(js_timer_overhead);
	BillingPolicy_Default()(&js_timer_clients[slot], js_start_times[slot], now);
	js_start_times[slot].SetToZero();

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Game Setup
//-----------------------------------------------------------------------------

// Immediately ends the current game (if any).
// params:
// returns:
JSBool EndGame(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_NO_PARAMS();

	EndGame();

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

//-----------------------------------------------------------------------------
// Misc. Engine Interface
//-----------------------------------------------------------------------------

// Return the global frames-per-second value.
// params:
// returns: FPS [int]
// notes:
// - This value is recalculated once a frame. We take special care to
//   filter it, so it is both accurate and free of jitter.
JSBool GetFps(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_NO_PARAMS();
	int freq = 0;
	if (g_frequencyFilter)
		freq = g_frequencyFilter->StableFrequency();
	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(freq));
	return JS_TRUE;
}


// Cause the game to exit gracefully.
// params:
// returns:
// notes:
// - Exit happens after the current main loop iteration ends
//   (since this only sets a flag telling it to end)
JSBool ExitProgram(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_NO_PARAMS();

	kill_mainloop();

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}


// Change the mouse cursor.
// params: cursor name [string] (i.e. basename of definition file and texture)
// returns:
// notes:
// - Cursors are stored in "art\textures\cursors"
JSBool SetCursor(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_PARAMS(1);
	g_CursorName = g_ScriptingHost.ValueToUCString(JS_ARGV(cx, vp)[0]);

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool GetGUIObjectByName(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_PARAMS(1);

	try
	{
		CStr name = ToPrimitive<CStr>(cx, JS_ARGV(cx, vp)[0]);
		IGUIObject* guiObj = g_GUI->FindObjectByName(name);
		if (guiObj)
			JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(guiObj->GetJSObject()));
		else
			JS_SET_RVAL(cx, vp, JSVAL_NULL);
		return JS_TRUE;
	}
	catch (PSERROR_Scripting&)
	{
		return JS_FALSE;
	}
}

//-----------------------------------------------------------------------------
// Miscellany
//-----------------------------------------------------------------------------

// Return the date/time at which the current executable was compiled.
// params: none (-> "date time (svn revision)") OR an integer specifying
//   what to display: 0 for date, 1 for time, 2 for svn revision
// returns: string with the requested timestamp info
// notes:
// - Displayed on main menu screen; tells non-programmers which auto-build
//   they are running. Could also be determined via .EXE file properties,
//   but that's a bit more trouble.
// - To be exact, the date/time returned is when scriptglue.cpp was
//   last compiled, but the auto-build does full rebuilds.
// - svn revision is generated by calling svnversion and cached in
//   lib/svn_revision.cpp. it is useful to know when attempting to
//   reproduce bugs (the main EXE and PDB should be temporarily reverted to
//   that revision so that they match user-submitted crashdumps).
JSBool GetBuildTimestamp(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_MAX_PARAMS(1);

	char buf[200];

	// see function documentation
	const int mode = argc? JSVAL_TO_INT(JS_ARGV(cx, vp)[0]) : -1;
	switch(mode)
	{
	case -1:
		sprintf_s(buf, ARRAY_SIZE(buf), "%s %s (%ls)", __DATE__, __TIME__, svn_revision);
		break;
	case 0:
		sprintf_s(buf, ARRAY_SIZE(buf), "%s", __DATE__);
		break;
	case 1:
		sprintf_s(buf, ARRAY_SIZE(buf), "%s", __TIME__);
		break;
	case 2:
		sprintf_s(buf, ARRAY_SIZE(buf), "%ls", svn_revision);
		break;
	}

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf)));
	return JS_TRUE;
}

#ifdef DEBUG
void DumpHeap(const char* basename, int idx, JSContext* cx)
{
	char filename[64];
	sprintf_s(filename, ARRAY_SIZE(filename), "%s.%03d.txt", basename, idx);
	OsPath pathname = psLogDir() / filename;
	FILE* f = sys_OpenFile(pathname, "w");
	debug_assert(f);
	JS_DumpHeap(cx, f, NULL, 0, NULL, (size_t)-1, NULL);
	fclose(f);
}
#endif

JSBool DumpHeaps(JSContext* cx, uintN argc, jsval* vp)
{
	UNUSED2(cx);
	UNUSED2(argc);

#ifdef DEBUG
	static int i = 0;

	if (ScriptingHost::IsInitialised())
		DumpHeap("gui", i, g_ScriptingHost.GetContext());
	if (g_Game)
		DumpHeap("sim", i, g_Game->GetSimulation2()->GetScriptInterface().GetContext());

	++i;
#else
	debug_warn(L"DumpHeaps only available in DEBUG mode");
#endif

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

//-----------------------------------------------------------------------------

// Is the game paused?
JSBool IsPaused(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_NO_PARAMS();

	if (!g_Game)
	{
		JS_ReportError(cx, "Game is not started");
		return JS_FALSE;
	}

	JS_SET_RVAL(cx, vp, g_Game->m_Paused ? JSVAL_TRUE : JSVAL_FALSE);
	return JS_TRUE;
}

// Pause/unpause the game
JSBool SetPaused(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_PARAMS( 1 );

	if (!g_Game)
	{
		JS_ReportError(cx, "Game is not started");
		return JS_FALSE;
	}

	try
	{
		g_Game->m_Paused = ToPrimitive<bool> (JS_ARGV(cx, vp)[0]);
	}
	catch (PSERROR_Scripting_ConversionFailed)
	{
		JS_ReportError(cx, "Invalid parameter to SetPaused");
	}

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// function table
//-----------------------------------------------------------------------------

// the JS interpreter expects the table to contain 5-tuples as follows:
// - name the function will be called as from script;
// - function which will be called;
// - number of arguments this function expects
// - Flags (deprecated, always zero)
// - Extra (reserved for future use, always zero)
//
// we simplify this a bit with a macro:
#define JS_FUNC(script_name, cpp_function, min_params) { script_name, cpp_function, min_params, 0 },

JSFunctionSpec ScriptFunctionTable[] =
{
	// Profiling
	JS_FUNC("startXTimer", StartJsTimer, 1)
	JS_FUNC("stopXTimer", StopJsTimer, 1)

	// Game Setup
	JS_FUNC("endGame", EndGame, 0)

	// VFS (external)
	JS_FUNC("buildDirEntList", JSI_VFS::BuildDirEntList, 1)
	JS_FUNC("getFileMTime", JSI_VFS::GetFileMTime, 1)
	JS_FUNC("getFileSize", JSI_VFS::GetFileSize, 1)
	JS_FUNC("readFile", JSI_VFS::ReadFile, 1)
	JS_FUNC("readFileLines", JSI_VFS::ReadFileLines, 1)
	JS_FUNC("archiveBuilderCancel", JSI_VFS::ArchiveBuilderCancel, 1)

	// Misc. Engine Interface
	JS_FUNC("exit", ExitProgram, 0)
	JS_FUNC("isPaused", IsPaused, 0)
	JS_FUNC("setPaused", SetPaused, 1)
	JS_FUNC("setCursor", SetCursor, 1)
	JS_FUNC("getFPS", GetFps, 0)
	JS_FUNC("getGUIObjectByName", GetGUIObjectByName, 1)

	// Miscellany
	JS_FUNC("buildTime", GetBuildTimestamp, 0)
	JS_FUNC("dumpHeaps", DumpHeaps, 0)

	// end of table marker
	{0}
};
#undef JS_FUNC


//-----------------------------------------------------------------------------
// property accessors
//-----------------------------------------------------------------------------

JSBool GetGameView(JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsid UNUSED(id), jsval* vp)
{
	if (g_Game)
		*vp = OBJECT_TO_JSVAL(g_Game->GetView()->GetScript());
	else
		*vp = JSVAL_NULL;
	return JS_TRUE;
}

JSBool GetRenderer(JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsid UNUSED(id), jsval* vp)
{
	if (CRenderer::IsInitialised())
		*vp = OBJECT_TO_JSVAL(g_Renderer.GetScript());
	else
		*vp = JSVAL_NULL;
	return JS_TRUE;
}


enum ScriptGlobalTinyIDs
{
	GLOBAL_SELECTION,
	GLOBAL_GROUPSARRAY,
	GLOBAL_CAMERA,
	GLOBAL_CONSOLE,
	GLOBAL_LIGHTENV
};

JSPropertySpec ScriptGlobalTable[] =
{
	{ "console"    , GLOBAL_CONSOLE,     JSPROP_PERMANENT|JSPROP_READONLY, JSI_Console::getConsole, 0 },
	{ "gameView"   , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetGameView, 0 },
	{ "renderer"   , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetRenderer, 0 },

	// end of table marker
	{ 0, 0, 0, 0, 0 },
};
