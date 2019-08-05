/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "JSInterface_Debug.h"

#include "i18n/L10n.h"
#include "lib/svn_revision.h"
#include "lib/debug.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>

/**
 * Microseconds since the epoch.
 */
double JSI_Debug::GetMicroseconds(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return JS_Now();
}

// Deliberately cause the game to crash.
// Currently implemented via access violation (read of address 0).
// Useful for testing the crashlog/stack trace code.
int JSI_Debug::Crash(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	debug_printf("Crashing at user's request.\n");
	return *(volatile int*)0;
}

void JSI_Debug::DebugWarn(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	debug_warn(L"Warning at user's request.");
}

void JSI_Debug::DisplayErrorDialog(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& msg)
{
	debug_DisplayError(msg.c_str(), DE_NO_DEBUG_INFO, NULL, NULL, NULL, 0, NULL, NULL);
}

// Return the date at which the current executable was compiled.
// - Displayed on main menu screen; tells non-programmers which auto-build
//   they are running. Could also be determined via .EXE file properties,
//   but that's a bit more trouble.
std::wstring JSI_Debug::GetBuildDate(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	UDate buildDate = g_L10n.ParseDateTime(__DATE__, "MMM d yyyy", icu::Locale::getUS());
	return wstring_from_utf8(g_L10n.LocalizeDateTime(buildDate, L10n::Date, icu::SimpleDateFormat::MEDIUM));
}

double JSI_Debug::GetBuildTimestamp(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	UDate buildDate = g_L10n.ParseDateTime(__DATE__ " " __TIME__, "MMM d yyyy HH:mm:ss", icu::Locale::getUS());
	if (buildDate)
		return buildDate / 1000.0;
	return std::time(nullptr);
}

// Return the revision number at which the current executable was compiled.
// - svn revision is generated by calling svnversion and cached in
//   lib/svn_revision.cpp. it is useful to know when attempting to
//   reproduce bugs (the main EXE and PDB should be temporarily reverted to
//   that revision so that they match user-submitted crashdumps).
std::wstring JSI_Debug::GetBuildRevision(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	std::wstring svnRevision(svn_revision);
	if (svnRevision == L"custom build")
		return wstring_from_utf8(g_L10n.Translate("custom build"));
	return svnRevision;
}

void JSI_Debug::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<double, &GetMicroseconds>("GetMicroseconds");
	scriptInterface.RegisterFunction<int, &Crash>("Crash");
	scriptInterface.RegisterFunction<void, &DebugWarn>("DebugWarn");
	scriptInterface.RegisterFunction<void, std::wstring, &DisplayErrorDialog>("DisplayErrorDialog");
	scriptInterface.RegisterFunction<std::wstring, &GetBuildDate>("GetBuildDate");
	scriptInterface.RegisterFunction<double, &GetBuildTimestamp>("GetBuildTimestamp");
	scriptInterface.RegisterFunction<std::wstring, &GetBuildRevision>("GetBuildRevision");
}
